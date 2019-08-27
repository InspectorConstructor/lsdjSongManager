#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndexList>
#include <QFileSystemModel>

#include <lsdj/sav.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void toHome();
    void toSplit();
    void toMerge();

    void resetSplit();
    void resetMerge();

    void updateSplitEnables();
    void updateMergeEnables();

    void mergeSongs(QStringList &songFiles);

private slots:

    void on_toSplitButton_clicked();

    void on_toMergeButton_clicked();

    void on_selectMergeDirectoryButton_clicked();

    void on_mergeDirectoryLine_textChanged(const QString &arg1);

    void on_doMergeButton_clicked();

    void on_mergeFileView_clicked(const QModelIndex &index);

    void on_resetButton_clicked();

    void on_splitSelectSavButton_clicked();

    void on_splitSavLine_textChanged(const QString &arg1);

    void on_splitSelectOutputButton_clicked();

    void on_actionHome_triggered();

    void on_actionSplit_triggered();

    void on_actionMerge_triggered();

    void on_splitSelectOutputLine_textChanged(const QString &arg1);

    void on_doSplitButton_clicked();

    void on_splitSongList_itemSelectionChanged();

private:
    Ui::MainWindow *ui;
    QFileSystemModel *m_model;
    lsdj_sav_t *m_split_sav;
};

#endif // MAINWINDOW_H
