/* This file is part of the KDE libraries
    Copyright (C) 2008 Alexander Dymo <adymo@kdevelop.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "kshortcutsdialog_p.h"

#include <QDir>
#include <QLabel>
#include <QMenu>
#include <QFile>
#include <QTextStream>
#include <QtXml/QDomDocument>
#include <QInputDialog>
#include <QFileDialog>

#include <kcombobox.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>
#include <kactioncollection.h>
#include <kmessagebox.h>
#include <kxmlguiclient.h>
#include <kmessagebox.h>

#include "kshortcutsdialog.h"
#include "kshortcutschemeshelper_p.h"

KShortcutSchemesEditor::KShortcutSchemesEditor(KShortcutsDialog *parent)
    :QGroupBox(i18n("Shortcut Schemes"), parent), m_dialog(parent)
{
    KConfigGroup group( KGlobal::config(), "Shortcut Schemes" );

    QStringList schemeFiles = KGlobal::dirs()->findAllResources("appdata", "*shortcuts.rc");
    QStringList schemes;
    schemes << "Default";
    foreach (QString schemeFile, schemeFiles)
    {
        schemes << schemeFile.remove(QRegExp("^.*/"+KGlobal::mainComponent().componentName())).
            remove("shortcuts.rc");
    }

    QString currentScheme = group.readEntry("Current Scheme", "Default");

    QHBoxLayout *l = new QHBoxLayout(this);
    l->setMargin(0);
    l->setSpacing(KDialog::spacingHint());

    QLabel *schemesLabel = new QLabel(i18n("Current Scheme:"), this);
    l->addWidget(schemesLabel);

    m_schemesList = new KComboBox(this);
    m_schemesList->setEditable(false);
    m_schemesList->addItems(schemes);
    m_schemesList->setCurrentIndex(m_schemesList->findText(currentScheme));
    schemesLabel->setBuddy(m_schemesList);
    l->addWidget(m_schemesList);

    m_newScheme = new KPushButton(i18n("New..."));
    l->addWidget(m_newScheme);

    m_deleteScheme = new KPushButton(i18n("Delete"));
    l->addWidget(m_deleteScheme);

    KPushButton *moreActions = new KPushButton(i18n("More Actions"));
    l->addWidget(moreActions);

    QMenu *moreActionsMenu = new QMenu(this);
    moreActionsMenu->addAction(i18n("Save As Scheme Defaults"),
        this, SLOT(saveAsDefaultsForScheme()));
    moreActionsMenu->addAction(i18n("Export Scheme..."),
        this, SLOT(exportShortcutsScheme()));

    moreActions->setMenu(moreActionsMenu);

    l->addStretch(1);

    connect(m_schemesList, SIGNAL(activated(const QString&)),
        this, SIGNAL(shortcutsSchemeChanged(const QString&)));
    connect(m_newScheme, SIGNAL(clicked()), this, SLOT(newScheme()));
    connect(m_deleteScheme, SIGNAL(clicked()), this, SLOT(deleteScheme()));
}

void KShortcutSchemesEditor::newScheme()
{
    bool ok;
    QString newName = QInputDialog::getText(this, i18n("Name for New Scheme"),
        i18n("Name for New Scheme:"), QLineEdit::Normal, "New Scheme", &ok);
    if (!ok || newName.isEmpty())
        return;

    if (m_schemesList->findText(newName) != -1)
    {
        KMessageBox::sorry(this, i18n("The scheme with this name already exists"));
        return;
    }

    QString newSchemeFileName = KShortcutSchemesHelper::applicationShortcutSchemeFileName(newName);

    QFile schemeFile(newSchemeFileName);
    if (!schemeFile.open(QFile::WriteOnly | QFile::Truncate))
        return;

    QDomDocument doc;
    QDomElement docElem = doc.createElement("kpartgui");
    doc.appendChild(docElem);
    QDomElement elem = doc.createElement("ActionProperties");
    docElem.appendChild(elem);

    QTextStream out(&schemeFile);
    out << doc.toString(4);

    m_schemesList->addItem(newName);
    m_schemesList->setCurrentIndex(m_schemesList->findText(newName));
    emit shortcutsSchemeChanged(newName);
}

void KShortcutSchemesEditor::deleteScheme()
{
    if (KMessageBox::questionYesNo(this,
            i18n("Do you really want to delete the scheme %1?\n\
Note that this will not remove any system wide shortcut schemes.", currentScheme())) == KMessageBox::No)
        return;

    //delete the scheme for the app itself
    QFile::remove(KShortcutSchemesHelper::applicationShortcutSchemeFileName(currentScheme()));

    //delete all scheme files we can find for xmlguiclients in the user directories
    foreach (KActionCollection *collection, m_dialog->actionCollections())
    {
        const KXMLGUIClient *client = collection->parentGUIClient();
        if (!client)
            continue;
        QFile::remove(KShortcutSchemesHelper::shortcutSchemeFileName(client, currentScheme()));
    }

    m_schemesList->removeItem(m_schemesList->findText(currentScheme()));
    emit shortcutsSchemeChanged(currentScheme());
}

QString KShortcutSchemesEditor::currentScheme()
{
    return m_schemesList->currentText();
}

void KShortcutSchemesEditor::exportShortcutsScheme()
{
    //ask user about dir
    QString exportTo = QFileDialog::getExistingDirectory(this, i18n("Export To Location"),
        QDir::currentPath());
    if (exportTo.isEmpty())
        return;

    QDir schemeRoot(exportTo);

    if (!schemeRoot.exists(exportTo))
    {
        KMessageBox::error(this, i18n("Could not export shortcuts scheme because the location is invalid."));
        return;
    }

    foreach (KActionCollection *collection, m_dialog->actionCollections())
    {
        const KXMLGUIClient *client = collection->parentGUIClient();
        if (!client) continue;
        QString fileDir = "shortcuts/share/apps/" + client->componentData().componentName() + "/";
        schemeRoot.mkpath(fileDir);
        KShortcutSchemesHelper::exportActionCollection(collection,
            currentScheme(), exportTo + "/" + fileDir);
    }
}

void KShortcutSchemesEditor::saveAsDefaultsForScheme()
{
    foreach (KActionCollection *collection, m_dialog->actionCollections())
        KShortcutSchemesHelper::exportActionCollection(collection, currentScheme());
}

