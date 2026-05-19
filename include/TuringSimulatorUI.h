#pragma once

#include <QAction>
#include <QMainWindow>
#include <QTabWidget>

#include "MachineEditorTab.h"

/// Main window for the Turing Machine Simulator with tabbed documents and menu bar.
class TuringSimulatorUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit TuringSimulatorUI(QWidget *parent = nullptr);
    ~TuringSimulatorUI() = default;

private slots:
    void createNewMachineTab();
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

    QTabWidget *machineTabs;
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
