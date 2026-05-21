#pragma once

#include <QAction>
#include <QMainWindow>
#include <QTabWidget>
#include <memory>

#include "MachineEditorTab.h"
#include "ThemeManager.h"

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
    void showStartPage();
    void createNewMachineTab();
    void createExampleMachineTab();
    void openMachineInNewTab();
    void saveCurrentMachine();
    void saveCurrentMachineAs();
    void openMachineFromDatabase();
    void saveCurrentMachineToDatabase();
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
    void showAboutAuthorsDialog();
    void toggleTheme();

private:
    void setupUI();
    void setupMenu();
    MachineEditorTab *currentMachineTab() const;
    QWidget *createStartPage();
    bool isStartPage(QWidget *widget) const;
    int addClosableTab(QWidget *tab, const QString &title, int insertIndex = -1);
    void addMachineTab(MachineEditorTab *tab, int insertIndex = -1);
    void addMachineTabFromStartPage(MachineEditorTab *tab);
    void openMachineFromDatabaseName(const QString &machineName);
    void closeTabAt(int index);
    void updateNewTabButtonPosition();

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
    QAction *toggleThemeAction;
    int untitledTabCounter;
    std::unique_ptr<ThemeManager> themeManager;
};
