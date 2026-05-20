#pragma once

#include <QObject>
#include <QString>

/// Manages application theme (light/dark mode) with mica effect
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class Theme
    {
        Light,
        Dark
    };

    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;

    /// Returns the current theme
    Theme currentTheme() const { return m_currentTheme; }

    /// Sets the theme and applies it to the application
    void setTheme(Theme theme);

    /// Toggles between light and dark mode
    void toggleTheme() { setTheme(m_currentTheme == Theme::Light ? Theme::Dark : Theme::Light); }

    /// Gets the stylesheet for the current theme
    QString getCurrentStylesheet() const;

    /// Gets the stylesheet for a specific theme
    static QString getStylesheet(Theme theme);

    /// Gets the palette for the current theme
    static QPalette getPalette(Theme theme);

    /// Applies a theme without checking if it's the same (used for initialization)
    void applyTheme(Theme theme);

signals:
    /// Emitted when theme changes
    void themeChanged(Theme theme);

private:
    /// Loads saved theme preference from settings
    void loadThemePreference();

    /// Saves theme preference to settings
    void saveThemePreference(Theme theme);

    Theme m_currentTheme;
};
