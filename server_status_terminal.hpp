#include <QListView>
#include <QMouseEvent>
#include <QClipboard>
#include <QFile>
#include <QTextStream>

#ifndef _SERVER_STATUS_TERMINAL_HPP
#define _SERVER_STATUS_TERMINAL_HPP


class ServerStatusTerminal: public QListView
{
public:
    explicit ServerStatusTerminal(QWidget *parent) : QListView(parent)
    {

    }
    [[nodiscard]] bool canCopy() const
    {
        return selectionModel()->hasSelection();
    }
    [[nodiscard]] bool canCut() const
    {
        return false;
    }
    [[nodiscard]] bool canPaste() const
    {
        return false;
    }
    [[nodiscard]] bool canClear() const
    {
        return (model()->rowCount() != 0);
    }
    [[nodiscard]] bool canSelectAll() const
    {
        auto rows = model()->rowCount();
        if (rows == 0) return false;
        auto srows = selectionModel()->selectedRows().count();
        if (srows == 0) return true;

        return (rows != srows);
    }
    void copy()
    {
        QStringListModel *pModel = dynamic_cast<QStringListModel *>(this->model());
        QClipboard *clipboard = QGuiApplication::clipboard();
        QString data;

        for (auto item : selectionModel()->selectedRows())
        {
            auto row = item.row();
            data += pModel->stringList().at(row);
        }

        clipboard->setText(data);
    }
    [[nodiscard]] bool canSave() const
    {
        return (model()->rowCount() != 0);
    }
protected:
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        QListView::mouseReleaseEvent(e);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if ((event->button() == Qt::LeftButton) && (!event->modifiers().testFlag(Qt::ControlModifier))) clearSelection();
        QAbstractItemView::mousePressEvent(event);
    }

public slots:
    void saveToFile(const QString &file)
    {
        if (file.isEmpty()) return;

        QStringListModel *pModel = dynamic_cast<QStringListModel* >(model());
        QStringList list = pModel->stringList();

        QFile textFile(file);
        if (textFile.open(QFile::WriteOnly | QFile::Truncate))
        {
            QTextStream out(&textFile);
            for (auto& s : list)
            {
                out << s << "\n";
            }
            textFile.close();
        }
        else
        {
            QMessageBox::critical(this, qAppName(), "The log file could not be saved!\n\n" + textFile.errorString());
        }
    }
};

#endif // _SERVER_STATUS_TERMINAL_HPP
