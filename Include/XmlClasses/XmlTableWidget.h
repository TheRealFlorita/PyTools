#ifndef XMLTABLEWIDGET_H
#define XMLTABLEWIDGET_H

#include <HdWidgets.h>
#include <XmlComboBoxDelegate.h>

#include <XmlAbstractObject.h>
#include <XmlModule.h>

#include <Settings.h>


class XmlTableWidget : public HdTableWidget, public XmlAbstractObject
{
    Q_OBJECT

private:
    QLocale loc, eur, cpp;
    QRegularExpression regexp;

public:

    explicit XmlTableWidget(QWidget *parent, pugi::xml_node node) : HdTableWidget(parent), XmlAbstractObject(node)
    {
        setTitle(getAttributeValue("name", QString()));

        regexp.setPattern("\\b([0-9]+[.,]{1}[0-9,.]+[Ee\\+\\-0-9]*)\\b");

        setLocale(loc);
        eur = QLocale(QLocale::Dutch, QLocale::Netherlands);
        eur.setNumberOptions(QLocale::OmitGroupSeparator);
        cpp = QLocale(QLocale::C);
        cpp.setNumberOptions(QLocale::OmitGroupSeparator);

        connect(this, &XmlTableWidget::rowInserted, this, &XmlTableWidget::updateInsertedRow);
        connect(this, &XmlTableWidget::columnInserted, this, &XmlTableWidget::updateInsertedColumn);
        connect(this, &XmlTableWidget::rowRemoved, this, &XmlTableWidget::updateRemovedRow);
        connect(this, &XmlTableWidget::columnRemoved, this, &XmlTableWidget::updateRemovedColumn);
        connect(this, &XmlTableWidget::cellChanged, this, &XmlTableWidget::updateChangedCell);

        /* Read data from xml node */
        initialize();
    }

    QString getTitle()
    {
        return getAttributeValue("name", QString());
    }

    void setTitle(QString title)
    {
        setAttributeValue("name", title);
    }

    void setRowCount(int rows)
    {
        HdTableWidget::setRowCount(rows);
        setAttributeValue("row-count", rowCount());
    }

    void setColumnCount(int columns)
    {
        HdTableWidget::setColumnCount(columns);
        setAttributeValue("column-count", columnCount());
    }

    void setRowsResizable(bool resizable)
    {
        HdTableWidget::setRowsResizable(resizable);
        setAttributeValue("rows-resizable", isRowsResizable());
    }

    void setColumnsResizable(bool resizable)
    {
        HdTableWidget::setColumnsResizable(resizable);
        setAttributeValue("columns-resizable", isColumnsResizable());
    }

    void setResizable(bool rows, bool columns)
    {
        setRowsResizable(rows);
        setColumnsResizable(columns);
    }

    void setXmlComboBoxDelegate(pugi::xml_node dnode)
    {
        XmlComboBoxDelegate *delegate = new XmlComboBoxDelegate(this, dnode);
        setItemDelegate(delegate);
    }

    void setXmlComboBoxDelegateForRow(pugi::xml_node dnode)
    {
        int row = QString(dnode.attribute("row").value()).trimmed().toInt();

        if ((row < rowCount()) && !rowsResizable)
            return;
        else if ((row < rowCount()) && rowsResizable)
            setRowCount(row+1);

        XmlComboBoxDelegate *delegate = new XmlComboBoxDelegate(this, dnode);
        delegate->setFillColor(settings.getColor("color/theme-light"));
        setItemDelegateForColumn(row, delegate);
    }

    void setXmlComboBoxDelegateForRow(int row, QString name, QStringList &items)
    {
        if ((row < rowCount()) && !rowsResizable)
            return;
        else if ((row < rowCount()) && rowsResizable)
            setRowCount(row+1);

        pugi::xml_node dnode = xmlNode.child("delegates").append_child("combo-box-delegate");
        dnode.append_attribute("name").set_value(name.toStdString().c_str());
        dnode.append_attribute("row").set_value(row);
        dnode.append_child("items");

        items.removeDuplicates();
        for (const QString &item: items)
            if (!item.trimmed().isEmpty())
                dnode.child("items").append_child("item").append_attribute("value").set_value(item.toStdString().c_str());

        setXmlComboBoxDelegateForRow(dnode);
    }

    void setXmlComboBoxDelegateForRows(QList<QPair<int, QStringList>> &rowItems)
    {
        for (int i = 0; i < rowItems.size(); ++i)
        {
            QString name = QString("rowdelegate%1").arg(rowItems[i].first);
            setXmlComboBoxDelegateForRow(rowItems[i].first, name, rowItems[i].second);
        }
    }

    void setXmlComboBoxDelegateForColumn(pugi::xml_node dnode)
    {
        int col = QString(dnode.attribute("column").value()).trimmed().toInt();

        if ((col >= columnCount()) && !columnsResizable)
            return;
        else if ((col >= columnCount()) && columnsResizable)
            setColumnCount(col+1);

        XmlComboBoxDelegate *delegate = new XmlComboBoxDelegate(this, dnode);
        delegate->setFillColor(settings.getColor("color/theme-light"));
        setItemDelegateForColumn(col, delegate);
    }

    void setXmlComboBoxDelegateForColumn(int col, QString name, QStringList &items)
    {
        if ((col >= columnCount()) && !columnsResizable)
            return;
        else if ((col >= columnCount()) && columnsResizable)
            setColumnCount(col+1);

        pugi::xml_node dnode = xmlNode.child("delegates").append_child("combo-box-delegate");
        dnode.append_attribute("name").set_value(name.toStdString().c_str());
        dnode.append_attribute("column").set_value(col);
        dnode.append_child("items");

        items.removeDuplicates();
        for (const QString &item: items)
            if (!item.trimmed().isEmpty())
                dnode.child("items").append_child("item").append_attribute("value").set_value(item.toStdString().c_str());

        setXmlComboBoxDelegateForColumn(dnode);
    }

    void setXmlComboBoxDelegateForColumns(QList<QPair<int, QStringList>> &columnItems)
    {
        for (int i = 0; i < columnItems.size(); ++i)
        {
            QString name = QString("coldelegate%1").arg(columnItems[i].first);
            setXmlComboBoxDelegateForColumn(columnItems[i].first, name, columnItems[i].second);
        }
    }

    void setLocale(QLocale locale)
    {
        loc = locale;
        loc.setNumberOptions(QLocale::OmitGroupSeparator);
    }

    QLocale getLocale()
    {
        return loc;
    }

    void initialize()
    {
        /* Temporarily disable all signals */
        bool blocked = signalsBlocked();
        blockSignals(true);

        /* Reset */
        clear();
        setRowCount(getAttributeValue("row-count", 5));
        setColumnCount(getAttributeValue("column-count", 3));
        setRowsResizable(getAttributeValue("rows-resizable", true));
        setColumnsResizable(getAttributeValue("columns-resizable", true));

        /* Read delegates */
        pugi::xpath_node_set delegate_nodes = xmlNode.select_nodes("./delegates/combo-box-delegate");
        bool has_row_delegates = false;
        bool has_column_delegates = false;
        for (size_t n = 0; n < delegate_nodes.size(); ++n)
        {
            pugi::xml_node delegate_node = delegate_nodes[n].node();

            if (delegate_node.attribute("column"))
            {
                has_column_delegates = true;
                setXmlComboBoxDelegateForColumn(delegate_node);
            }
            else if (delegate_node.attribute("row"))
            {
                has_row_delegates = true;
                setXmlComboBoxDelegateForRow(delegate_node);
            }
        }

        /* Correct resizability */
        if (has_row_delegates)
            setRowsResizable(false);
        if (has_column_delegates)
            setColumnsResizable(false);

        /* Read column data*/
        if (!xmlNode.child("columns") && xmlNode.child("delegates"))
            xmlNode.insert_child_after("columns", xmlNode.child("delegates"));
        else if (!xmlNode.child("columns"))
            xmlNode.prepend_child("columns");

        QList<int> columns;
        bool sort_columns = false;
        int max_j = 0;
        pugi::xpath_node_set column_nodes = xmlNode.select_nodes("./columns/column");
        for (size_t n = 0; n < column_nodes.size(); ++n)
        {
            pugi::xml_node column_node = column_nodes[n].node();

            /* Check consistency of column index */
            bool ok = false;
            int j = QString(column_node.attribute("j").value()).toInt(&ok);

            if (j < 0 || !ok || columns.contains(j) || !column_node.attribute("j"))
            {
                column_node.parent().remove_child(column_node);
                continue;
            }
            columns.append(j);

            if (j < columns.last())
                sort_columns = true;

            QString header = QString(column_node.attribute("header").value());

            if ((horizontalHeaderItem(j) != Q_NULLPTR) && !header.trimmed().isEmpty())
                horizontalHeaderItem(j)->setText(header);
            else if (!header.trimmed().isEmpty())
                setHorizontalHeaderItem(j, new QTableWidgetItem(header));
        }

        /* Reorder columns */
        if (sort_columns && columns.length() > 1)
        {
            std::sort(columns.begin(), columns.end());
            for (int j: columns)
            {
                QString xpath = "./columns/column[@j='" + QString::number(j) + "']";
                pugi::xml_node column_node = xmlNode.select_node(xpath.toStdString().c_str()).node();
                column_node.parent().append_move(column_node);
            }
        }

        /* Read row data*/
        if (!xmlNode.child("rows"))
            xmlNode.insert_child_after("rows", xmlNode.child("columns"));

        QList<int> rows;
        bool sort_rows = false;
        int max_i = 0;
        pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./rows/row");
        for (size_t n = 0; n < row_nodes.size(); ++n)
        {
            pugi::xml_node row_node = row_nodes[n].node();

            bool ok = false;
            int i = QString(row_node.attribute("i").value()).toInt(&ok);
            max_i = std::max(max_i, i);

            /* Check consistency of row index */
            if (i < 0 || !ok || rows.contains(i) || !row_node.attribute("i"))
            {
                row_node.parent().remove_child(row_node);
                continue;
            }
            rows.append(i);

            if (i < rows.last())
                sort_rows = true;

            /* Header attribute */
            if (!row_node.attribute("header"))
                row_node.insert_attribute_after("header", row_node.attribute("i"));

            QString header = QString(row_node.attribute("header").value());

            if ((verticalHeaderItem(i) != Q_NULLPTR) && !header.trimmed().isEmpty())
                verticalHeaderItem(i)->setText(header);
            else if (!header.trimmed().isEmpty())
                setVerticalHeaderItem(i, new QTableWidgetItem(header));

            /* Items */
            QList<int> items;
            bool sort_items = false;
            pugi::xpath_node_set item_nodes = row_node.select_nodes("./item");
            for (size_t n = 0; n < item_nodes.size(); ++n)
            {
                pugi::xml_node item_node = item_nodes[n].node();

                /* Check consistency of item row index */
                if (!item_node.attribute("i"))
                    item_node.prepend_attribute("i").set_value(i);
                else
                {
                    int row = QString(item_node.attribute("i").value()).toInt(&ok);
                    if (!ok || row != i)
                    {
                        item_node.parent().remove_child(item_node);
                        continue;
                    }
                }

                /* Check consistency of item column index */
                int j = QString(item_node.attribute("j").value()).toInt(&ok);
                if (!ok || items.contains(j) || !item_node.attribute("j"))
                {
                    item_node.parent().remove_child(item_node);
                    continue;
                }
                items.append(j);

                if (j < items.last())
                    sort_items = true;              

                /* Check value */
                if (item_node.attribute("value").empty())
                {
                    item_node.parent().remove_child(item_node);
                    continue;
                }

                setItemText(i,j,formatNumeric(item_node.attribute("value").value()));

                /* Fix compatibility */
                if (item_node.attribute("is-editable"))
                {
                    if (!item_node.attribute("read-only") && !QString(item_node.attribute("is-editable").value()).toLower().replace("true","1").replace("false","0").toInt())
                        item_node.insert_attribute_after("read-only", item_node.attribute("is-editable")).set_value("1");
                    item_node.remove_attribute("is-editable");
                }

                /* Set read-only property */
                if (item_node.attribute("read-only"))
                {
                    if (QString(item_node.attribute("read-only").value()).toLower().replace("true","1").replace("false","0").toInt())
                        item(i,j)->setFlags(item(i,j)->flags() & (~Qt::ItemIsEditable));
                    else
                        item_node.remove_attribute("read-only");
                }
            }

            /* Reorder items */
            if (sort_items && items.length() > 1)
            {
                std::sort(items.begin(), items.end());
                for (int j: items)
                {
                    QString xpath = "./item[@j='" + QString::number(j) + "']";
                    pugi::xml_node item_node = row_node.select_node(xpath.toStdString().c_str()).node();
                    item_node.parent().append_move(item_node);
                }
            }
        }

        /* Reorder rows */
        if (sort_rows && rows.length() > 1)
        {
            std::sort(rows.begin(), rows.end());
            for (int i: rows)
            {
                QString xpath = "./rows/row[@i='" + QString::number(i) + "']";
                pugi::xml_node row_node = xmlNode.select_node(xpath.toStdString().c_str()).node();
                row_node.parent().append_move(row_node);
            }
        }

        /* Set column count */
        if (columnsResizable && max_j >= columnCount())
            setColumnCount(max_j+1);

        /* Set row count */
        if (rowsResizable && max_i >= rowCount())
            setRowCount(max_i+1);

         /* Tool tip */
        if (getAttribute("tool-tip"))
            setToolTip(getAttributeValue("tool-tip", QString()).trimmed());

        blockSignals(blocked);
        updateViewport();
    }

    static XmlTableWidget* createFromXmlNode(XmlModule *parent, pugi::xml_node node, int rowheight)
    {
        XmlTableWidget *tableWidget = new XmlTableWidget(parent, node);
        tableWidget->setDynamicRowHeight(rowheight);
        tableWidget->setDynamicMinimumColumnWidth(100);
        tableWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        tableWidget->setUseViewportSizeHintHeight(true);
        connect(parent, &XmlModule::dpiScaleChanged, tableWidget, &XmlTableWidget::updateDpiScale);

        if (parent->getDpiScale() != 1.0)
            tableWidget->updateDpiScale(parent->getDpiScale());

        return tableWidget;
    }

private:

    void updateChangedCell(int i, int j)
    {
        if (i >= rowCount() || j >= columnCount())
            return;

        /* Set text format to locale */
        if (item(i,j) != Q_NULLPTR)
        {
            bool blocked = signalsBlocked();
            blockSignals(true);
            item(i,j)->setText(formatNumeric(item(i,j)->text()));
            blockSignals(blocked);
        }

        /* Update xml */
        pugi::xml_node rows_node = xmlNode.child("rows");
        QString rpath = "./rows/row[@i='" + QString::number(i) + "']";
        pugi::xml_node row_node = xmlNode.select_node(rpath.toStdString().c_str()).node();
        QString ipath = rpath + "/item[@j='" + QString::number(j) + "']";
        pugi::xml_node item_node = xmlNode.select_node(ipath.toStdString().c_str()).node();
        QString value;

        /* Set xml text format to cpp locale */
        if (item(i,j) != Q_NULLPTR)
            value = formatNumeric(item(i,j)->text(), true);

        if (value.isEmpty())
        {
            /* Delete item node if value is empty */
            if (item_node)
                item_node.parent().remove_child(item_node);

            /* Delete row node entire row is empty */
            if (row_node)
                if (int(row_node.select_nodes("item").size()) == 0 && QString(row_node.attribute("header").value()).trimmed() == "")
                    row_node.parent().remove_child(row_node);
        }
        else
        {
            /* Insert new row node at correct location */
            if (!row_node)
            {
                pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./rows/row");
                for (size_t n = 0; n < row_nodes.size(); ++n)
                {
                    pugi::xml_node child_node = row_nodes[n].node();

                    if (QString(child_node.attribute("i").value()).toInt() > i)
                    {
                        row_node = rows_node.insert_child_before("row", child_node);
                        row_node.append_attribute("i").set_value(i);
                        row_node.append_attribute("header");
                        break;
                    }
                }

                if (!row_node)
                {
                    row_node = rows_node.append_child("row");
                    row_node.append_attribute("i").set_value(i);
                    row_node.append_attribute("header");
                }

                if (item_node)
                {
                    qDebug() << "invalid item node!";
                    item_node.parent().remove_child(item_node);
                }

                item_node = row_node.append_child("item");
                item_node.append_attribute("i").set_value(i);
                item_node.append_attribute("j").set_value(j);
                item_node.append_attribute("value").set_value(value.toStdString().c_str());
            }

            /* Insert new item node at correct location */
            else if (!item_node)
            {
                pugi::xpath_node_set item_nodes = row_node.select_nodes("./item");
                for (size_t n = 0; n < item_nodes.size(); ++n)
                {
                    pugi::xml_node child_node = item_nodes[n].node();
                    if (QString(child_node.attribute("j").value()).toInt() > j)
                    {
                        item_node = row_node.insert_child_before("item", child_node);
                        break;
                    }
                }

                if (!item_node)
                    item_node = row_node.append_child("item");

                item_node.append_attribute("i").set_value(i);
                item_node.append_attribute("j").set_value(j);
                item_node.append_attribute("value").set_value(value.toStdString().c_str());
            }

            /* If item already exists */
            else
                item_node.attribute("value").set_value(value.toStdString().c_str());
        }
    }

    void updateInsertedRow(int row)
    {
        pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./rows/row");
        for (size_t n = 0; n < row_nodes.size(); ++n)
        {
            pugi::xml_node row_node = row_nodes[n].node();
            int i = QString(row_node.attribute("i").value()).toInt();

            if (i < row)
                continue;

            row_node.attribute("i").set_value(i+1);
            pugi::xpath_node_set item_nodes = row_node.select_nodes("./item");
            for (size_t n = 0; n < item_nodes.size(); ++n)
                item_nodes[n].node().attribute("i").set_value(i+1);
        }

        setAttributeValue("row-count", rowCount());
    }

    void updateInsertedColumn(int column)
    {
        pugi::xpath_node_set column_nodes = xmlNode.select_nodes("./columns/column");
        for (size_t n = 0; n < column_nodes.size(); ++n)
        {
            pugi::xml_node column_node = column_nodes[n].node();
            int j = QString(column_node.attribute("j").value()).toInt();

            if (j < column)
                continue;

            column_node.attribute("j").set_value(j+1);
        }

        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./rows/row/item");
        for (size_t n = 0; n < item_nodes.size(); ++n)
        {
            pugi::xml_node item_node = item_nodes[n].node();
            int j = QString(item_node.attribute("j").value()).toInt();

            if (j >= column)
                item_node.attribute("j").set_value(j+1);
        }

        setAttributeValue("column-count", columnCount());
    }

    void updateRemovedRow(int row)
    {
        pugi::xpath_node_set row_nodes = xmlNode.select_nodes("./rows/row");
        for (size_t n = 0; n < row_nodes.size(); ++n)
        {
            pugi::xml_node row_node = row_nodes[n].node();
            int i = QString(row_node.attribute("i").value()).toInt();

            if (i == row)
                row_node.parent().remove_child(row_node);
            else if (i > row)
            {
                row_node.attribute("i").set_value(i-1);
                pugi::xpath_node_set item_nodes = row_node.select_nodes("./item");
                for (size_t j = 0; j < item_nodes.size(); ++j)
                    item_nodes[j].node().attribute("i").set_value(i-1);
            }
        }

        setAttributeValue("row-count", rowCount());
    }

    void updateRemovedColumn(int column)
    {
        pugi::xpath_node_set column_nodes = xmlNode.select_nodes("./columns/column");
        for (size_t n = 0; n < column_nodes.size(); ++n)
        {
            pugi::xml_node column_node = column_nodes[n].node();
            int j = QString(column_node.attribute("j").value()).toInt();

            if (j == column)
                column_node.parent().remove_child(column_node);
            else if (j > column)
                column_node.attribute("j").set_value(j-1);
        }

        pugi::xpath_node_set item_nodes = xmlNode.select_nodes("./rows/row/item");
        for (size_t n = 0; n < item_nodes.size(); ++n)
        {
            pugi::xml_node item_node = item_nodes[n].node();
            int j = QString(item_node.attribute("j").value()).toInt();

            if (j == column)
                item_node.parent().remove_child(item_node);
            else if (j > column)
                item_node.attribute("j").set_value(j-1);
        }

        setAttributeValue("column-count", columnCount());
    }

    QString formatNumeric(QString text, bool useCppLocale=false)
    {
        if (text.trimmed().isEmpty())
            return text;

        QString formatted = QString(text);

        /* Set locale formatting on double/float */
        QRegularExpressionMatch match;
        while ((match = regexp.match(text)).hasMatch())
        {
            text.remove(match.captured(0));

            /* Convert to double using locale format */
            bool ok;
            double d = loc.toDouble(match.captured(0), &ok);

            /* Try again using default format */
            if(!ok)
                d = cpp.toDouble(match.captured(0), &ok);

            /* Try again using european format */
            if(!ok)
                d = eur.toDouble(match.captured(0), &ok);

            /* Convert back to string */
            if (ok)
            {
                QString s;
                if (useCppLocale)
                {
                    s = cpp.toString(d);
                    if (!s.contains(cpp.decimalPoint()) && !s.contains("e", Qt::CaseInsensitive))
                        s += QString(cpp.decimalPoint()) + "0";
                }
                else
                {
                    s = loc.toString(d);
                    if (!s.contains(loc.decimalPoint()) && !s.contains("e", Qt::CaseInsensitive))
                        s += QString(loc.decimalPoint()) + "0";
                }

                formatted.replace(match.captured(0), s);
            }
        }
        return formatted;
    }
};

#endif
