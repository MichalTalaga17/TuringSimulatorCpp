#include "ui/GeminiDialogStyle.h"

QString geminiDialogStyleSheet()
{
    return R"(
        QDialog {
            background-color: #f8fafc;
            color: #111827;
        }
        QLabel#dialogTitle {
            color: #111827;
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#dialogSubtitle {
            color: #475569;
            font-size: 12px;
        }
        QLabel#fieldLabel {
            color: #334155;
            font-size: 12px;
            font-weight: 700;
        }
        QFrame#settingsFrame {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 8px;
        }
        QLineEdit,
        QTextEdit,
        QComboBox {
            background-color: #ffffff;
            border: 1px solid #cbd5e1;
            border-radius: 8px;
            color: #111827;
            padding: 8px;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
        }
        QLineEdit {
            min-height: 30px;
            padding: 6px 8px;
        }
        QLineEdit:focus,
        QTextEdit:focus,
        QComboBox:focus {
            border: 1px solid #2563eb;
        }
        QComboBox::drop-down {
            border: none;
            width: 28px;
        }
        QCheckBox {
            color: #111827;
            font-weight: 600;
        }
        QPushButton {
            border-radius: 8px;
            min-height: 32px;
            padding: 6px 16px;
            font-weight: 700;
        }
        QPushButton#primaryButton {
            background-color: #2563eb;
            border: 1px solid #2563eb;
            color: #ffffff;
        }
        QPushButton#primaryButton:hover {
            background-color: #1d4ed8;
            border-color: #1d4ed8;
        }
        QPushButton#primaryButton:disabled {
            background-color: #bfdbfe;
            border-color: #bfdbfe;
            color: #eff6ff;
        }
        QPushButton#secondaryButton {
            background-color: #ffffff;
            border: 1px solid #cbd5e1;
            color: #334155;
        }
        QPushButton#secondaryButton:hover {
            background-color: #f1f5f9;
        }
    )";
}
