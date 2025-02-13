/*************************************************************************
 * This file is part of tuna
 * git.vrsal.xyz/alex/tuna
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "output_edit_dialog.hpp"
#include "../query/music_source.hpp"
#include "../util/constants.hpp"
#include "../util/format.hpp"
#include "tuna_gui.hpp"
#include "ui_output_edit_dialog.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <cmath>
#ifdef _WIN32
#    include <QTextStream>
#endif

output_edit_dialog::output_edit_dialog(edit_mode m, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::output_edit_dialog)
    , m_mode(m)
{
    ui->setupUi(this);
    m_tuna = dynamic_cast<tuna_gui*>(parent);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(ui->browse, &QPushButton::clicked, this, &output_edit_dialog::browse_clicked);
    connect(ui->txt_format, &QLineEdit::textChanged, this, &output_edit_dialog::format_changed);
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &output_edit_dialog::accept_clicked);

    ui->lbl_format_old->setVisible(false);
    ui->lbl_format_error->setVisible(false);
    ui->lbl_format_old->setStyleSheet("QLabel { color: red;"
                                      "font-weight: bold; }");
    ui->lbl_format_error->setStyleSheet("QLabel { color: red;"
                                        "font-weight: bold; }");
    ui->table_format->setColumnWidth(0, 110);
    ui->table_format->setColumnWidth(1, 140);
    ui->table_format->setColumnWidth(2, 110);

    auto const& spec = format::get_specifiers();
    auto rows = std::ceil(spec.size() / 2.);
    for (int i = 0; i < rows; i++)
        ui->table_format->insertRow(i);

    for (int i = 0; i < int(spec.size()); i += 2) {
        ui->table_format->setItem(i / 2, 0, new QTableWidgetItem(spec[i]->get_id()));
        ui->table_format->setItem(i / 2, 1, new QTableWidgetItem(spec[i]->get_name()));
        if (i + 1 < int(spec.size())) {
            ui->table_format->setItem(i / 2, 2, new QTableWidgetItem(spec[i + 1]->get_id()));
            ui->table_format->setItem(i / 2, 3, new QTableWidgetItem(spec[i + 1]->get_name()));
        }
    }

    if (m == edit_mode::modify) {
        QString format, path;
        bool log_mode = false;
        m_tuna->get_selected_output(format, path, log_mode);
        ui->txt_format->setText(format);
        ui->txt_path->setText(path);
        ui->cb_logmode->setChecked(log_mode);
    }
}

output_edit_dialog::~output_edit_dialog()
{
    delete ui;
}

static inline bool is_valid_file(const QString& file)
{
    bool result = false;
#ifdef _WIN32
    QFile f(file); /* On NTFS file checks don't work unless the file exists */
    result = f.open(QIODevice::WriteOnly | QIODevice::Text);
    f.close();
#else
    QFile test(file);

    result = test.open(QFile::OpenModeFlag::ReadWrite);
    if (result)
        test.close();
#endif
    return result;
}

void output_edit_dialog::accept_clicked()
{
    bool empty = ui->txt_format->text().isEmpty();
    bool valid = is_valid_file(ui->txt_path->text());

    if (empty || !valid) {
        QMessageBox::warning(this, T_OUTPUT_ERROR_TITLE, T_OUTPUT_ERROR);
    }

    if (m_mode == edit_mode::create) {
        m_tuna->add_output(ui->txt_format->text(), ui->txt_path->text(), ui->cb_logmode->isChecked());
    } else {
        m_tuna->edit_output(ui->txt_format->text(), ui->txt_path->text(), ui->cb_logmode->isChecked());
    }
}

void output_edit_dialog::format_changed(const QString& format)
{
    auto src = music_sources::selected_source();
    if (src)
        ui->lbl_format_error->setVisible(!src->valid_format(format));

    static QRegularExpression e("%[a-zA-Z](\\[[0-9]+\\])?");
    Q_ASSERT(e.isValid());
    ui->lbl_format_old->setVisible(e.match(format).hasMatch());
}

void output_edit_dialog::browse_clicked()
{
    QString path = QFileDialog::getSaveFileName(this, tr(T_SELECT_SONG_FILE), QDir::home().path(),
        tr(FILTER("Text file", "*.txt")));
    ui->txt_path->setText(path);
}
