// vim: noet ts=8 sw=8
/* This file is part of the KDE libraries
    Copyright (C) 1998 Mark Donohoe <donohoe@kde.org>
    Copyright (C) 2001 Ellis Whitehead <ellis@kde.org>
    Copyright (C) 2007 Andreas Hartmetz <ahartmetz@gmail.com>

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

#include "kkeysequencewidget.h"
#include "kkeysequencewidget_p.h"

#include "kkeyserver.h"

#include <QKeyEvent>
#include <QTimer>
#include <QHBoxLayout>
#include <QToolButton>
#include <QApplication>

#include <kdebug.h>
#include <kglobalaccel.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kshortcut.h>
#include <kaction.h>
#include <kactioncollection.h>

class KKeySequenceWidgetPrivate
{
public:
	KKeySequenceWidgetPrivate(KKeySequenceWidget *q);

	void init();

	static QKeySequence appendToSequence(const QKeySequence& seq, int keyQt);
	static bool isOkWhenModifierless(int keyQt);

	void updateShortcutDisplay();
	void startRecording();

	/**
	 * Conflicts the key sequence @a seq with a current standard
	 * shortcut?
	 */
	bool conflictWithStandardShortcuts(const QKeySequence &seq);

	/**
	 * Conflicts the key sequence @a seq with a current local
	 * shortcut?
	 */
	bool conflictWithLocalShortcuts(const QKeySequence &seq);

	/**
	 * Conflicts the key sequence @a seq with a current global
	 * shortcut?
	 */
	bool conflictWithGlobalShortcuts(const QKeySequence &seq);

	/**
	 * Get permission to steal the shortcut @seq from the standard shortcut @a std.
	 */
	bool stealStandardShortcut(KStandardShortcut::StandardShortcut std, const QKeySequence &seq);

	bool checkAgainstStandardShortcuts() const
	{
		return checkAgainstShortcutTypes & KKeySequenceWidget::StandardShortcuts;
	}

	bool checkAgainstGlobalShortcuts() const
	{
		return checkAgainstShortcutTypes & KKeySequenceWidget::GlobalShortcuts;
	}

	bool checkAgainstLocalShortcuts() const
	{
		return checkAgainstShortcutTypes & KKeySequenceWidget::LocalShortcuts;
	}

//private slot
	void doneRecording(bool validate = true);

//members
	KKeySequenceWidget *const q;
	QHBoxLayout *layout;
	KKeySequenceButton *keyButton;
	QToolButton *clearButton;

	QKeySequence keySequence;
	QKeySequence oldKeySequence;
	QTimer modifierlessTimeout;
	bool allowModifierless;
	uint nKey;
	uint modifierKeys;
	bool isRecording;

	//! Check the key sequence against KStandardShortcut::find()
	KKeySequenceWidget::ShortcutTypes checkAgainstShortcutTypes;

    /**
     * The list of action to check against for conflict shortcut
     */
    QList<QAction*> checkList; // deprecated

    /**
     * The list of action collections to check against for conflict shortcut
     */
    QList<KActionCollection*> checkActionCollections;

	/**
	 * The action to steal the shortcut from.
	 */
	KAction* stealAction;

	bool stealShortcut(QAction *item, const QKeySequence &seq);
	void wontStealShortcut(QAction *item, const QKeySequence &seq);

};

KKeySequenceWidgetPrivate::KKeySequenceWidgetPrivate(KKeySequenceWidget *q)
	: q(q)
	 ,layout(NULL)
	 ,keyButton(NULL)
	 ,clearButton(NULL)
	 ,allowModifierless(false)
	 ,nKey(0)
	 ,modifierKeys(0)
	 ,isRecording(false)
	 ,checkAgainstShortcutTypes(KKeySequenceWidget::LocalShortcuts & KKeySequenceWidget::GlobalShortcuts)
	 ,stealAction(NULL)
{}


bool KKeySequenceWidgetPrivate::stealShortcut(QAction *item, const QKeySequence &seq)
{
	QString title = i18n("Key Conflict");
	QString message = i18n("The '%1' key combination has already been allocated to the \"%2\" action.\n"
            "Do you want to reassign it from that action to the current one?", seq.toString(QKeySequence::NativeText), item->text().remove('&'));

	if (KMessageBox::warningContinueCancel(q, message, title, KGuiItem(i18n("Reassign"))) != KMessageBox::Continue)
		return false;

	return true;
}

void KKeySequenceWidgetPrivate::wontStealShortcut(QAction *item, const QKeySequence &seq)
{
	QString title( i18n( "Shortcut conflict" ) );
	QString msg( i18n( "<qt>The '%1' key combination is already used by the <b>%2</b> action.<br>"
			"Please select a different one.</qt>", seq.toString(QKeySequence::NativeText) , item->text().remove('&') ) );
	KMessageBox::sorry( q, msg );
}


KKeySequenceWidget::KKeySequenceWidget(QWidget *parent)
 : QWidget(parent),
   d(new KKeySequenceWidgetPrivate(this))
{
	d->init();
	connect(d->keyButton, SIGNAL(clicked()), this, SLOT(captureKeySequence()));
	connect(d->clearButton, SIGNAL(clicked()), this, SLOT(clearKeySequence()));
	connect(&d->modifierlessTimeout, SIGNAL(timeout()), this, SLOT(doneRecording()));
	//TODO: how to adopt style changes at runtime?
	/*QFont modFont = d->clearButton->font();
	modFont.setStyleHint(QFont::TypeWriter);
	d->clearButton->setFont(modFont);*/
	d->updateShortcutDisplay();
}


void KKeySequenceWidgetPrivate::init()
{
	layout = new QHBoxLayout(q);

	keyButton = new KKeySequenceButton(this, q);
	keyButton->setFocusPolicy(Qt::StrongFocus);
	keyButton->setIcon(KIcon("configure"));
	layout->addWidget(keyButton);

	clearButton = new QToolButton(q);
	layout->addWidget(clearButton);

	if (qApp->isLeftToRight())
		clearButton->setIcon(KIcon("edit-clear-locationbar-rtl"));
	else
		clearButton->setIcon(KIcon("edit-clear-locationbar-ltr"));
}


KKeySequenceWidget::~KKeySequenceWidget ()
{
	delete d;
}


KKeySequenceWidget::ShortcutTypes KKeySequenceWidget::checkForConflictsAgainst() const
{
	return d->checkAgainstShortcutTypes;
}


void KKeySequenceWidget::setCheckForConflictsAgainst(ShortcutTypes types)
{
	d->checkAgainstShortcutTypes = types;
}

void KKeySequenceWidget::setModifierlessAllowed(bool allow)
{
	d->allowModifierless = allow;
}


bool KKeySequenceWidget::isKeySequenceAvailable(const QKeySequence &keySequence) const
{
	return ! ( d->conflictWithLocalShortcuts(keySequence)
	           || d->conflictWithGlobalShortcuts(keySequence)
	           || d->conflictWithStandardShortcuts(keySequence));
}


bool KKeySequenceWidget::isModifierlessAllowed()
{
	return d->allowModifierless;
}


void KKeySequenceWidget::setClearButtonShown(bool show)
{
	d->clearButton->setVisible(show);
}

void KKeySequenceWidget::setCheckActionList(const QList<QAction*> &checkList) // deprecated
{
    d->checkList = checkList;
    Q_ASSERT(d->checkActionCollections.isEmpty()); // don't call this method if you call setCheckActionCollections!
}

void KKeySequenceWidget::setCheckActionCollections(const QList<KActionCollection *>& actionCollections)
{
    d->checkActionCollections = actionCollections;
}

//slot
void KKeySequenceWidget::captureKeySequence()
{
	d->startRecording();
}


QKeySequence KKeySequenceWidget::keySequence() const
{
	return d->keySequence;
}


//slot
void KKeySequenceWidget::setKeySequence(const QKeySequence &seq, Validation validate)
{
	// oldKeySequence holds the key sequence before recording started, if setKeySequence()
	// is called while not recording then set oldKeySequence to the existing sequence so
	// that the keySequenceChanged() signal is emitted if the new and previous key
	// sequences are different
	if (!d->isRecording)
		d->oldKeySequence = d->keySequence;

	d->keySequence = seq;
	d->doneRecording(validate == Validate);
}


//slot
void KKeySequenceWidget::clearKeySequence()
{
	setKeySequence(QKeySequence());
}

//slot
void KKeySequenceWidget::applyStealShortcut()
{
	kDebug();

	if(d->stealAction) {

		kDebug() << "Stealing shortcut for " << d->stealAction->objectName();

		// Stealing a shortcut means setting it to an empty one
		d->stealAction->setShortcut(KShortcut(), KAction::ActiveShortcut);

                // Find the collection where stealAction belongs
                KActionCollection* parentCollection = 0;
                foreach(KActionCollection* collection, d->checkActionCollections) {
                    if (collection->actions().contains(d->stealAction)) {
                        parentCollection = collection;
                        break;
                    }
                }
                if (parentCollection)
			parentCollection->writeSettings();
	}
	d->stealAction = NULL;
}

void KKeySequenceButton::setText(const QString &text)
{
	QPushButton::setText(text);
	//setFixedSize( sizeHint().width()+12, sizeHint().height()+8 );
}


void KKeySequenceWidgetPrivate::startRecording()
{
	nKey = 0;
	modifierKeys = 0;
	oldKeySequence = keySequence;
	keySequence = QKeySequence();
	isRecording = true;
	keyButton->grabKeyboard();

	if (!QWidget::keyboardGrabber()) {
		kWarning() << "Failed to grab the keyboard! Most likely qt's nograb option is active";
	}

	keyButton->setDown(true);
	updateShortcutDisplay();
}


void KKeySequenceWidgetPrivate::doneRecording(bool validate)
{
	modifierlessTimeout.stop();
	isRecording = false;
	keyButton->releaseKeyboard();
	keyButton->setDown(false);
	stealAction = NULL;

	if (keySequence != oldKeySequence && validate && !q->isKeySequenceAvailable(keySequence)) {
		keySequence = oldKeySequence;
		updateShortcutDisplay();
		return;
	}

	updateShortcutDisplay();
	if (keySequence != oldKeySequence)
		emit q->keySequenceChanged(keySequence);
	return;
}


bool KKeySequenceWidgetPrivate::conflictWithGlobalShortcuts(const QKeySequence &keySequence)
{
	if (!checkAgainstShortcutTypes & KKeySequenceWidget::GlobalShortcuts) {
		return false;
	}

	QStringList conflicting = KGlobalAccel::findActionNameSystemwide(keySequence);
	if (!conflicting.isEmpty()) {
		if (!KGlobalAccel::promptStealShortcutSystemwide(q, conflicting, keySequence)) {
			return true;
		}
		// The user approved stealing the shortcut. We have to steal
		// it immediately because KAction::setGlobalShortcut() refuses
		// to set a global shortcut that is already used. There is no
		// error it just silently fails. So be nice because this is
		// most likely the first action that is done in the slot
		// listening to keySequenceChanged().
		KGlobalAccel::stealShortcutSystemwide(keySequence);
	}
	return false;
}


bool KKeySequenceWidgetPrivate::conflictWithLocalShortcuts(const QKeySequence &keySequence)
{
	if (!checkAgainstShortcutTypes & KKeySequenceWidget::LocalShortcuts) {
		return false;
	}

	// We have actions both in the deprecated checkList and the
	// checkActionCollections list. Add all the actions to a single list to
	// be able to process them in a single loop below.
	// Note that this can't be done in setCheckActionCollections(), because we
	// keep pointers to the action collections, and between the call to
	// setCheckActionCollections() and this function some actions might already be
	// removed from the collection again.
	QList<QAction*> allActions;
	allActions += checkList;
	foreach(KActionCollection* collection, checkActionCollections) {
		allActions += collection->actions();
	}

	//find conflicting shortcuts with existing actions
	foreach(QAction * qaction , allActions ) {
		KAction *kaction=qobject_cast<KAction*>(qaction);
		if(kaction) {
			if(kaction->shortcut().contains(keySequence)) {
				// A conflict with a KAction. If that action is configurable
				// ask the user what to do. If not reject this keySequence.
				if(kaction->isShortcutConfigurable ()) {
					if(stealShortcut(kaction, keySequence)) {
						stealAction = kaction;
						break;
					} else {
						return true;
					}
				} else {
					wontStealShortcut(kaction, keySequence);
					return true;
				}
			}
		} else {
			if(qaction->shortcut() == keySequence) {
				// A conflict with a QAction. Don't know why :-( but we won't
				// steal from that kind of actions.
				wontStealShortcut(qaction, keySequence);
				return true;
			}
		}
	}

	return false;
}


bool KKeySequenceWidgetPrivate::conflictWithStandardShortcuts(const QKeySequence &keySequence)
{
	if (!checkAgainstShortcutTypes & KKeySequenceWidget::StandardShortcuts) {
		return false;
	}

	KStandardShortcut::StandardShortcut ssc = KStandardShortcut::find(keySequence);
	if (ssc != KStandardShortcut::AccelNone && !stealStandardShortcut(ssc, keySequence)) {
		return true;
	}
	return false;
}


bool KKeySequenceWidgetPrivate::stealStandardShortcut(KStandardShortcut::StandardShortcut std, const QKeySequence &seq)
{
    QString title = i18n("Conflict with Standard Application Shortcut");
    QString message = i18n("The '%1' key combination is also used for the standard action "
                           "\"%2\" that some applications use.\n"
                           "Do you really want to use it as a global shortcut as well?",
                           seq.toString(QKeySequence::NativeText), KStandardShortcut::name(std));

    if (KMessageBox::warningContinueCancel(q, message, title, KGuiItem(i18n("Reassign"))) != KMessageBox::Continue) {
        return false;
    }
    return true;
}


void KKeySequenceWidgetPrivate::updateShortcutDisplay()
{
	//empty string if no non-modifier was pressed
	QString s = keySequence.toString(QKeySequence::NativeText);
	s.replace('&', QLatin1String("&&"));

	if (isRecording) {
		// Display modifiers for the first key in the QKeySequence
		if (nKey == 0) {
			if (modifierKeys) {
				if (modifierKeys & Qt::META)  s += KKeyServer::modToStringUser(Qt::META) + '+';
#if defined(Q_WS_MAC)
                if (modifierKeys & Qt::ALT)   s += KKeyServer::modToStringUser(Qt::ALT) + '+';
				if (modifierKeys & Qt::CTRL)  s += KKeyServer::modToStringUser(Qt::CTRL) + '+';
#elif defined(Q_WS_X11)
				if (modifierKeys & Qt::CTRL)  s += KKeyServer::modToStringUser(Qt::CTRL) + '+';
				if (modifierKeys & Qt::ALT)   s += KKeyServer::modToStringUser(Qt::ALT) + '+';
#endif
				if (modifierKeys & Qt::SHIFT) s += KKeyServer::modToStringUser(Qt::SHIFT) + '+';
			} else
				s = i18nc("What the user inputs now will be taken as the new shortcut", "Input");
		}
		//make it clear that input is still going on
		s.append(" ...");
	}

	if (s.isEmpty())
		s = i18nc("No shortcut defined", "None");

	s.prepend(' ');
	s.append(' ');
	keyButton->setText(s);
}


KKeySequenceButton::~KKeySequenceButton()
{
}


//prevent Qt from special casing Tab and Backtab
bool KKeySequenceButton::event (QEvent* e)
{
	if (d->isRecording && e->type() == QEvent::KeyPress) {
		keyPressEvent(static_cast<QKeyEvent *>(e));
		return true;
	}

	// The shortcut 'alt+c' ( or any other dialog local action shortcut )
	// ended the recording and triggered the action associated with the
	// action. In case of 'alt+c' ending the dialog.  It seems that those
	// ShortcutOverride events get sent even if grabKeyboard() is active.
	if (d->isRecording && e->type() == QEvent::ShortcutOverride) {
		e->accept();
		return true;
	}

	return QPushButton::event(e);
}


void KKeySequenceButton::keyPressEvent(QKeyEvent *e)
{
	int keyQt = e->key();
	if (keyQt == -1) {
		// Qt sometimes returns garbage keycodes, I observed -1, if it doesn't know a key.
		// We cannot do anything useful with those (several keys have -1, indistinguishable)
		// and QKeySequence.toString() will also yield a garbage string.
		return;
	}
	uint newModifiers = e->modifiers() & (Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META);

	//don't have the return or space key appear as first key of the sequence when they
	//were pressed to start editing - catch and them and imitate their effect
	if (!d->isRecording && ((keyQt == Qt::Key_Return || keyQt == Qt::Key_Space))) {
		d->startRecording();
		d->modifierKeys = newModifiers;
		d->updateShortcutDisplay();
		return;
	}

	if (!d->isRecording)
		return QPushButton::keyPressEvent(e);

	e->accept();

	if (d->nKey == 0)
		d->modifierKeys = newModifiers;

	switch(keyQt) {
	case Qt::Key_AltGr: //or else we get unicode salad
		return;
	case Qt::Key_Shift:
	case Qt::Key_Control:
	case Qt::Key_Alt:
	case Qt::Key_Meta:
	case Qt::Key_Menu: //unused (yes, but why?)
		// If we are editing the first key in the sequence,
		// display modifier keys which are held down
		if(d->nKey == 0)
			d->updateShortcutDisplay();
		break;
	default:
		//Shift is not a modifier in the sense of Ctrl/Alt/WinKey
		if (!(d->modifierKeys & ~Qt::SHIFT)) {
			if (!KKeySequenceWidgetPrivate::isOkWhenModifierless(keyQt) && !d->allowModifierless) {
				return;
			} else {
				d->modifierlessTimeout.start(600);
			}
		}

		if (keyQt) {
			if (d->nKey == 0) {
				d->keySequence =
				  KKeySequenceWidgetPrivate::appendToSequence(d->keySequence,
				                                              keyQt | d->modifierKeys);
			} else {
				d->keySequence =
				  KKeySequenceWidgetPrivate::appendToSequence(d->keySequence, keyQt);
			}

			d->nKey++;
			if (d->nKey >= 4) {
				d->doneRecording();
				return;
			}
			d->updateShortcutDisplay();
		}
	}
}


void KKeySequenceButton::keyReleaseEvent(QKeyEvent *e)
{
	if (e->key() == -1) {
		// ignore garbage, see keyPressEvent()
		return;
	}

	if (!d->isRecording)
		return QPushButton::keyReleaseEvent(e);

	e->accept();

	uint newModifiers = e->modifiers() & (Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META);

	//if a modifier that belongs to the shortcut was released...
	if ((newModifiers & d->modifierKeys) < d->modifierKeys) {
		if (d->nKey == 0) {
			d->modifierKeys = newModifiers;
			d->updateShortcutDisplay();
		} else
			d->doneRecording();
	}
}


//static
QKeySequence KKeySequenceWidgetPrivate::appendToSequence(const QKeySequence& seq, int keyQt)
{
	switch (seq.count()) {
	case 0:
		return QKeySequence(keyQt);
	case 1:
		return QKeySequence(seq[0], keyQt);
	case 2:
		return QKeySequence(seq[0], seq[1], keyQt);
	case 3:
		return QKeySequence(seq[0], seq[1], seq[2], keyQt);
	default:
		return seq;
	}
}


//static
bool KKeySequenceWidgetPrivate::isOkWhenModifierless(int keyQt)
{
	//this whole function is a hack, but especially the first line of code
	if (QKeySequence(keyQt).toString().length() == 1)
		return false;

	switch (keyQt) {
	case Qt::Key_Return:
	case Qt::Key_Space:
	case Qt::Key_Tab:
	case Qt::Key_Backtab: //does this ever happen?
	case Qt::Key_Backspace:
	case Qt::Key_Delete:
		return false;
	default:
		return true;
	}
}

#include "kkeysequencewidget.moc"
#include "kkeysequencewidget_p.moc"
