#pragma once

#include <QAction>
#include <QMainWindow>
#include <QTabWidget>

#include "MachineEditorTab.h"

class QPushButton;
class QResizeEvent;

/// Main window for the Turing Machine Simulator with tabbed documents and menu bar.
class TuringSimulatorUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit TuringSimulatorUI(QWidget *parent = nullptr);
    ~TuringSimulatorUI() = default;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void createNewMachineTab();
    void createExampleMachineTab();
    void openMachineInNewTab();
    void saveCurrentMachine();
    void saveCurrentMachineAs();
    void closeCurrentMachineTab();
    void validateCurrentMachine();
    void resetCurrentMachineToExample();
    void stepCurrentMachine();
    void toggleCurrentMachineRun();
    void resetCurrentMachine();
    void selectPreviousTab();
    void selectNextTab();
    void updateMenuState();
    void showAboutDialog();

private:
    void setupUI();
    void setupMenu();
    MachineEditorTab *currentMachineTab() const;
    void addMachineTab(MachineEditorTab *tab);
    void updateNewTabButtonPosition();
    void showNewTabMenu();

    QTabWidget *machineTabs;
    QPushButton *newTabButton;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *closeTabAction;
    QAction *validateAction;
    QAction *resetExampleAction;
    QAction *stepAction;
    QAction *runAction;
    QAction *resetAction;
    QAction *previousTabAction;
    QAction *nextTabAction;
    int untitledTabCounter;
};
