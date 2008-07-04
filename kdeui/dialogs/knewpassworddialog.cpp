// vi: ts=8 sts=4 sw=4
/* This file is part of the KDE libraries
   Copyright (C) 1998 Pietro Iglio <iglio@fub.it>
   Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
   Copyright (C) 2004,2005 Andrew Coles <andrew_coles@yahoo.co.uk>
   Copyright (C) 2007 Michaël Larouche <larouche@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "knewpassworddialog.h"

#include <QtGui/QApplication>
#include <QtGui/QProgressBar>
#include <QtCore/QRegExp>
#include <QtCore/QSize>
#include <QtCore/QString>

#include <kapplication.h>
#include <kglobal.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klineedit.h>
#include <ktitlewidget.h>

#include "ui_knewpassworddialog.h"

class KNewPasswordDialog::KNewPasswordDialogPrivate
{
public:
    KNewPasswordDialogPrivate( KNewPasswordDialog *parent )
        : q( parent ),
         minimumPasswordLength(0), passwordStrengthWarningLevel(1),reasonablePasswordLength(8)
    {}

    void init();
    void _k_textChanged();

    KNewPasswordDialog *q;

    int minimumPasswordLength;
    int passwordStrengthWarningLevel;
    int reasonablePasswordLength;

    QString pass;

    Ui::KNewPasswordDialog ui;
};


void KNewPasswordDialog::KNewPasswordDialogPrivate::init()
{
    q->setButtons( Ok | Cancel );
    q->showButtonSeparator( true );
    q->setDefaultButton( Ok );

    ui.setupUi( q->mainWidget() );

    ui.labelIcon->setPixmap( KIcon("dialog-password").pixmap(96, 96) );
    ui.labelMatch->setHidden(true);

    const QString strengthBarWhatsThis(i18n("The password strength meter gives an indication of the security "
            "of the password you have entered.  To improve the strength of "
            "the password, try:\n"
            " - using a longer password;\n"
            " - using a mixture of upper- and lower-case letters;\n"
            " - using numbers or symbols, such as #, as well as letters."));
    ui.labelStrengthMeter->setWhatsThis(strengthBarWhatsThis);
    ui.strengthBar->setWhatsThis(strengthBarWhatsThis);

    connect( ui.linePassword, SIGNAL(textChanged(const QString&)), q, SLOT(_k_textChanged()) );
    connect( ui.lineVerifyPassword, SIGNAL(textChanged(const QString&)), q, SLOT(_k_textChanged()) );

    _k_textChanged();
}


void KNewPasswordDialog::KNewPasswordDialogPrivate::_k_textChanged()
{
    const bool match = ui.linePassword->text() == ui.lineVerifyPassword->text();

    const int minPasswordLength = q->minimumPasswordLength();

    if ( ui.linePassword->text().length() < minPasswordLength) {
        q->enableButtonOk(false);
    } else {
        q->enableButtonOk( match );
    }

    if ( match && !q->allowEmptyPasswords() && ui.linePassword->text().isEmpty()) {
        ui.labelMatch->setPixmap( KIcon("dialog-error") );
        ui.labelMatch->setText( i18n("Password is empty") );
    }
    else {
        if ( ui.linePassword->text().length() < minPasswordLength ) {
            ui.labelMatch->setPixmap( KIcon("dialog-error") );
            ui.labelMatch->setText(i18np("Password must be at least 1 character long", "Password must be at least %1 characters long", minPasswordLength));
        } else {
            ui.labelMatch->setPixmap( match ? KIcon("dialog-ok") : KIcon("dialog-error") );
            // "ok" icon should probably be "dialog-success", but we don't have that icon in KDE 4.0
            ui.labelMatch->setText( match? i18n("Passwords match")
                :i18n("Passwords do not match") );
        }
    }

      // Password strength calculator
      // Based on code in the Master Password dialog in Firefox
      // (pref-masterpass.js)
      // Original code triple-licensed under the MPL, GPL, and LGPL
      // so is license-compatible with this file

    const double lengthFactor = reasonablePasswordLength / 8.0;

    int pwlength = (int) ( ui.linePassword->text().length()/ lengthFactor);
    if (pwlength > 5) {
        pwlength = 5;
    }

    const QRegExp numRxp("[0-9]", Qt::CaseSensitive, QRegExp::RegExp);
    int numeric = (int) (ui.linePassword->text().count(numRxp) / lengthFactor);
    if (numeric > 3) {
        numeric = 3;
    }

    const QRegExp symbRxp("\\W", Qt::CaseInsensitive, QRegExp::RegExp);
    int numsymbols = (int) (ui.linePassword->text().count(symbRxp) / lengthFactor);
    if (numsymbols > 3) {
        numsymbols = 3;
    }

    const QRegExp upperRxp("[A-Z]", Qt::CaseSensitive, QRegExp::RegExp);
    int upper = (int) (ui.linePassword->text().count(upperRxp) / lengthFactor);
    if (upper > 3) {
        upper = 3;
    }

    int pwstrength=((pwlength*10)-20) + (numeric*10) + (numsymbols*15) + (upper*10);

    if ( pwstrength < 0 ) {
        pwstrength = 0;
    }

    if ( pwstrength > 100 ) {
        pwstrength = 100;
    }
    ui.strengthBar->setValue(pwstrength);
}

/*
 * Password dialog.
 */

KNewPasswordDialog::KNewPasswordDialog( QWidget *parent)
    : KDialog(parent), d(new KNewPasswordDialogPrivate(this))
{
    d->init();
}


KNewPasswordDialog::~KNewPasswordDialog()
{
    delete d;
}


void KNewPasswordDialog::setPrompt(const QString &prompt)
{
    d->ui.labelPrompt->setText(prompt);
}


QString KNewPasswordDialog::prompt() const
{
    return d->ui.labelPrompt->text();
}


void KNewPasswordDialog::setPixmap(const QPixmap &pixmap)
{
    d->ui.labelIcon->setPixmap(pixmap);
    d->ui.labelIcon->setFixedSize( d->ui.labelIcon->sizeHint() );
}


QPixmap KNewPasswordDialog::pixmap() const
{
    return *d->ui.labelIcon->pixmap();
}


void KNewPasswordDialog::accept()
{
    if ( d->ui.linePassword->text() != d->ui.lineVerifyPassword->text() ) {
        d->ui.labelMatch->setPixmap( KTitleWidget::ErrorMessage );
        d->ui.labelMatch->setText( i18n("You entered two different "
                "passwords. Please try again.") );

        d->ui.linePassword->clear();
        d->ui.lineVerifyPassword->clear();
        return;
    }
    if (d->ui.strengthBar && d->ui.strengthBar->value() < d->passwordStrengthWarningLevel) {
        int retVal = KMessageBox::warningContinueCancel(this,
                i18n(   "The password you have entered has a low strength. "
                        "To improve the strength of "
                        "the password, try:\n"
                        " - using a longer password;\n"
                        " - using a mixture of upper- and lower-case letters;\n"
                        " - using numbers or symbols as well as letters.\n"
                        "\n"
                        "Would you like to use this password anyway?"),
                i18n("Low Password Strength"));
        if (retVal == KMessageBox::Cancel) return;
    }
    if ( !checkPassword(d->ui.linePassword->text()) ) {
        return;
    }
    d->pass = d->ui.linePassword->text();
    emit newPassword( d->pass );
    KDialog::accept();
}


void KNewPasswordDialog::setAllowEmptyPasswords(bool allowed)
{
    setMinimumPasswordLength( allowed ? 0 : 1 );
    d->_k_textChanged();
}


bool KNewPasswordDialog::allowEmptyPasswords() const
{
    return d->minimumPasswordLength == 0;
}

void KNewPasswordDialog::setMinimumPasswordLength(int minLength)
{
    d->minimumPasswordLength = minLength;
    d->_k_textChanged();
}

int KNewPasswordDialog::minimumPasswordLength() const
{
    return d->minimumPasswordLength;
}

void KNewPasswordDialog::setMaximumPasswordLength(int maxLength)
{
    d->ui.linePassword->setMaxLength(maxLength);
    d->ui.lineVerifyPassword->setMaxLength(maxLength);
}

int KNewPasswordDialog::maximumPasswordLength() const
{
    return d->ui.linePassword->maxLength();
}

// reasonable password length code contributed by Steffen Mthing

void KNewPasswordDialog::setReasonablePasswordLength(int reasonableLength)
{

    if (reasonableLength < 1) {
        reasonableLength = 1;
    }
    if (reasonableLength >= maximumPasswordLength()) {
        reasonableLength = maximumPasswordLength();
    }

    d->reasonablePasswordLength = reasonableLength;

}

int KNewPasswordDialog::reasonablePasswordLength() const
{
    return d->reasonablePasswordLength;
}


void KNewPasswordDialog::setPasswordStrengthWarningLevel(int warningLevel)
{
    if (warningLevel < 0) {
        warningLevel = 0;
    }
    if (warningLevel > 99) {
        warningLevel = 99;
    }
    d->passwordStrengthWarningLevel = warningLevel;
}

int KNewPasswordDialog::passwordStrengthWarningLevel() const
{
    return d->passwordStrengthWarningLevel;
}

QString KNewPasswordDialog::password() const 
{
    return d->pass;
}

bool KNewPasswordDialog::checkPassword(const QString &)
{
    return true;
}

#include "knewpassworddialog.moc"

// kate: space-indent on; indent-width 4; encoding utf-8; replace-tabs on;
