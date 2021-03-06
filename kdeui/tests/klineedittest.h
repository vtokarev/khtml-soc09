#ifndef _KLINEEDITTEST_H
#define _KLINEEDITTEST_H

#include <QtGui/QWidget>
#include <QtCore/QPointer>

class QString;
class QPushButton;

class KLineEdit;

class KLineEditTest : public QWidget
{
    Q_OBJECT

public:
   KLineEditTest( QWidget *parent=0 );
   ~KLineEditTest();
   KLineEdit* lineEdit() const { return m_lineedit; }

public Q_SLOTS:
   virtual void show ();

private Q_SLOTS:
   void quitApp();
   void slotHide();
   void slotEnable( bool );
   void slotReadOnly( bool );
   void slotPassword( bool );
   void slotReturnPressed();
   void resultOutput( const QString& );
   void slotReturnPressed( const QString& );
   void slotClickMessage(bool click);
protected:
   QPointer<KLineEdit> m_lineedit;
   QPushButton* m_btnExit;
   QPushButton* m_btnReadOnly;
   QPushButton* m_btnPassword;
   QPushButton* m_btnEnable;
   QPushButton* m_btnHide;
   QPushButton* m_btnClickMessage;
};

#endif
