#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <lsdj/project.h>

#include <QString>
#include <QPixmap>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_model(new QFileSystemModel),
    m_split_sav(nullptr)
{
    ui->setupUi(this);

    connect(m_model, &QFileSystemModel::directoryLoaded, this, &MainWindow::directoryLoaded);

    // todo: use statusBar to indicate status
    // ui->statusBar->showMessage("welcome", 5000);

    // make the lemon
    QPixmap bkgnd(":/images/lemon.jpg");
    bkgnd = bkgnd.scaled(ui->lemon->size(), Qt::KeepAspectRatio);
    ui->lemon->setPixmap(bkgnd);

    resetSplit();
    resetMerge();
    toHome();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_model;
    if (m_split_sav)
        lsdj_sav_free(m_split_sav);
}

void MainWindow::toHome()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->resetButton->setEnabled(false);
}

void MainWindow::toSplit()
{
    ui->resetButton->setEnabled(true);
    resetSplit();
    updateSplitEnables();
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::toMerge()
{
    ui->resetButton->setEnabled(true);
    resetMerge();
    updateMergeEnables();
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::resetSplit()
{
    ui->splitSelectOutputLine->clear();
    ui->splitSongList->clear();
    ui->splitSavLine->clear();
    updateSplitEnables();
}

void MainWindow::resetMerge()
{
    ui->mergeDirectoryLine->clear();
    ui->mergeFileView->reset();
    ui->mergeFileView->setEnabled(false);
    updateMergeEnables();
}


void MainWindow::updateSplitEnables()
{
    bool haveSav = false, validSav = false,
            haveOutput = false, validOutput = false,
            anySongsSelected = false;

    haveSav = ! ui->splitSavLine->text().isEmpty();
    haveOutput = ! ui->splitSelectOutputLine->text().isEmpty();

    if (haveSav)
        validSav = QFile::exists(ui->splitSavLine->text());

    if (haveOutput)
    {
        QFileInfo info(ui->splitSelectOutputLine->text());
        if (info.exists())
            validOutput = info.isDir();
    }

    anySongsSelected = !ui->splitSongList->selectionModel()->selectedRows().empty();

    qDebug() << "haveSav =" << haveSav
             << " validSav =" << validSav
             << " haveOutput =" << haveOutput
             << " validOutput =" << validOutput
             << " anySongsSelected" << anySongsSelected;

    ui->doSplitButton->setEnabled(validOutput && validSav && anySongsSelected);
}

void MainWindow::updateMergeEnables()
{
    bool en = false;
    if (QFile::exists(ui->mergeDirectoryLine->text()))
    {
        auto model_p = ui->mergeFileView->selectionModel();
        if (model_p != nullptr)
            en = !model_p->selectedIndexes().empty();
    }
    ui->doMergeButton->setEnabled(en);
}

bool lsdj_check_err(lsdj_error_t *err)
{
    if (err)
    {
        QString msg = "Error: ";
        msg.append(lsdj_error_get_c_str(err));
        qCritical() << msg;
        QMessageBox::critical(nullptr, "Merge failed", msg);
        lsdj_error_free(err);
        return true;
    }
    return false;
}

void MainWindow::mergeSongs(QStringList &songFiles)
{
    lsdj_error_t *err = nullptr;
    lsdj_sav_t *sav = lsdj_sav_new(&err);
    if (lsdj_check_err(err))
        return;

    if (songFiles.length() > 16)
    {
        QMessageBox::critical(nullptr, "Merge failed", "too many lsdsng selected!");
        return;
    }
    else if (songFiles.empty())
    {
        QMessageBox::warning(nullptr, "Merge failed", "please select at least one .lsdsng!");
        return;
    }

    int i = 0;
    for (QString &string : songFiles)
    {
        lsdj_project_t *project = lsdj_project_read_lsdsng_from_file(string.toLocal8Bit(), &err);
        if (lsdj_check_err(err))
            return;

        lsdj_sav_set_project(sav, i++, project, &err);
        if (lsdj_check_err(err))
            return;
    }

    // SUCCESS
    QString selected = QFileDialog::getSaveFileName(this, "Name your new .sav file", QString(), "*.sav");

    if (selected.isEmpty())
        return;

    // dialog.selectedFiles().first();
    lsdj_sav_write_to_file(sav, selected.toLocal8Bit(), &err);
    if (lsdj_check_err(err))
        return;

    QMessageBox::information(this, "Merge Success", "Successfully merged " +
                                                    QString::number(i) + " .lsdsng files!");
}

void MainWindow::on_toSplitButton_clicked()
{
    toSplit();
}

void MainWindow::on_toMergeButton_clicked()
{
    toMerge();
}

void MainWindow::on_selectMergeDirectoryButton_clicked()
{
    QFileDialog d(this);
    d.setFileMode(QFileDialog::Directory);

    d.setOption(QFileDialog::ShowDirsOnly, true);

    if (d.exec())
    {
        QString outputPath = d.selectedFiles().first();
        ui->mergeDirectoryLine->setText(outputPath);

        if (QFile::exists(outputPath)) // extra safe!
        {
            ui->mergeFileView->setEnabled(true);
            ui->mergeFileView->setModel(m_model);
            m_model->setFilter(QDir::Files);
            m_model->setReadOnly(true);
            m_model->setRootPath(outputPath);
            m_model->setNameFilterDisables(false); // true shows disabled files, false hides 'em.
            m_model->setNameFilters({"*.lsdsng", outputPath});
            ui->mergeFileView->setRootIndex(m_model->index(outputPath));
        }
        else
        {
            ui->mergeFileView->setEnabled(false);
            QMessageBox::critical(nullptr, "Hmm...", "Selected directory doesn't exist.");
        }
    }
}

void MainWindow::on_mergeDirectoryLine_textChanged(const QString &newText)
{
    Q_UNUSED(newText)
    // textChanged signal is emitted whenever the text changes: even programmatically!

    // todo: set timeout to do the following:
    {
        updateMergeEnables();
    }
}

void MainWindow::on_doMergeButton_clicked()
{
    QModelIndexList list = ui->mergeFileView->selectionModel()->selectedRows();
    QStringList fileList;

    foreach(const QModelIndex &index, list)
    {
        fileList.append( m_model->filePath(index));
    }

    qDebug() << "SELECTION: ";
    qDebug() << fileList.join(", ");

    mergeSongs(fileList);
}

void MainWindow::on_mergeFileView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index)    
    updateMergeEnables();
}

void MainWindow::on_resetButton_clicked()
{
    toHome();
}

void MainWindow::on_splitSelectSavButton_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter("*.sav");
    if (dialog.exec())
    {
        ui->splitSavLine->setText(dialog.selectedFiles().first());
    }
}

void MainWindow::on_splitSavLine_textChanged(const QString &arg1)
{
    ui->splitSongList->clear();
    updateSplitEnables();

    QFileInfo savFile(arg1);
    if (savFile.exists())
    {
        lsdj_error_t *err = nullptr;
        m_split_sav = lsdj_sav_read_from_file(arg1.toLocal8Bit(), &err);
        if (err)
        {
            QString msg = "error opening sav: ";
            msg += QString::fromLocal8Bit(lsdj_error_get_c_str(err));

            QMessageBox::warning(this, "Invlid sav", msg);
            lsdj_error_free(err);
        }
        else
        {
            for (int i = 0 ; i < 32 ; ++i)
            {
                char name[LSDJ_PROJECT_NAME_LENGTH+1];
                memset(name, 0, LSDJ_PROJECT_NAME_LENGTH+1);
                lsdj_project_t *project = lsdj_sav_get_project(m_split_sav, i);
                lsdj_project_get_name(project, name, LSDJ_PROJECT_NAME_LENGTH+1);

                QString q = QString("%1").arg(i, 2, 16, QChar('0'));
                q.append(": \t").append(name);
                QListWidgetItem *item = new QListWidgetItem(ui->splitSongList);
                item->setData(Qt::UserRole, name);
                item->setText(q);
                //ui->splitSongList->insertItem(i, q);
                //auto item = ui->splitSongList->item(i);
                if (name[0] != '\0')
                {
                    item->setSelected(true);
                }
                else
                {
                    item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled);
                }
                ui->splitSongList->insertItem(i, item);

                ui->splitSongList->focusWidget();
            }

            if (ui->splitSelectOutputLine->text().isEmpty())
                ui->splitSelectOutputLine->setText(savFile.path());
        }
    }
}

void MainWindow::on_splitSelectOutputButton_clicked()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    if (dialog.exec())
    {
        ui->splitSelectOutputLine->setText(dialog.selectedFiles().first());
    }
}

void MainWindow::on_actionHome_triggered()
{
    toHome();
}

void MainWindow::on_actionSplit_triggered()
{
    toSplit();
}

void MainWindow::on_actionMerge_triggered()
{
    toMerge();
}

void MainWindow::on_splitSelectOutputLine_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    updateSplitEnables();
}

void MainWindow::on_doSplitButton_clicked()
{
    bool success = true;
    auto selected = ui->splitSongList->selectionModel()->selectedRows();
    int i;
    for (i = 0; i < selected.length(); ++i)
    {
        const QModelIndex &modelIndex = selected.at(i);
        int idx = modelIndex.row();

        QString filename = QString::number(idx);
        filename += '_';
        filename += modelIndex.data(Qt::UserRole).toString();
        filename += ".lsdsng";

        QString path = ui->splitSelectOutputLine->text();
        path += QDir::separator();
        path += filename;

        // ask before overwriting
        if (QFile::exists(path))
        {
            auto x = QMessageBox::question(this, "Overwrite file " + filename, "overwrite " + filename + "?");

            if (x != QMessageBox::Yes)
                continue;
        }


        lsdj_error_t *err = nullptr;
        lsdj_project_t *project = lsdj_sav_get_project(m_split_sav, idx);
        lsdj_project_write_lsdsng_to_file(project, path.toLocal8Bit(), &err);
        if (err)
        {
            qCritical() << QString::fromLocal8Bit(lsdj_error_get_c_str(err));
            lsdj_error_free(err);
            success = false;
            break;
        }
    }

    if (success)
        QMessageBox::information(this, "Split Success", "Successfully created "
                                                        + QString::number(selected.length())
                                                        + " .lsdsng files.");
    else
        QMessageBox::information(this, "Split Failure", "Split failed after creating "
                                                        + QString::number(i)
                                                        + " .lsdsng files.");
}

void MainWindow::on_splitSongList_itemSelectionChanged()
{
    updateSplitEnables();
}

void MainWindow::directoryLoaded(const QString &path)
{
    qDebug() << "LOAD";
    this->updateMergeEnables();
}
