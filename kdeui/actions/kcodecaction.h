/*
   kcodecaction.h

    Copyright (c) 2003      Jason Keirstead   <jason@keirstead.org>
    Copyright (c) 2003-2006 Michel Hermier    <michel.hermier@gmail.com>
    Copyright (c) 2007      Nick Shaforostoff <shafff@ukr.net>

    ********************************************************************
    *                                                                  *
    * This library is free software; you can redistribute it and/or    *
    * modify it under the terms of the GNU Lesser General Public       *
    * License as published by the Free Software Foundation; either     *
    * version 2 of the License, or (at your option) any later version. *
    *                                                                  *
    * This library is distributed in the hope that it will be useful,  *
    * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
    * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
    * GNU Lesser General Public License for more details.              *
    *                                                                  *
    * You should have received a copy of the GNU Lesser General Public *
    * License along with this library; if not, write to the            *
    * Free Software Foundation, Inc., 51 Franklin Street,              *
    * Fifth Floor, Boston, MA  02110-1301  USA                         *
    *                                                                  *
    ********************************************************************
*/
#ifndef KCODECACTION_H
#define KCODECACTION_H

#include <kencodingdetector.h>
#include <kselectaction.h>

/**
 *  @short Action for selecting one of several QTextCodec.
 *
 *  This action shows up a submenu with a list of the available codecs on the system.
 */
class KDEUI_EXPORT KCodecAction
	: public KSelectAction
{
	Q_OBJECT

	Q_PROPERTY(QString codecName READ currentCodecName WRITE setCurrentCodec)
	Q_PROPERTY(int codecMib READ currentCodecMib)

public:
	explicit KCodecAction(QObject *parent,bool showAutoOptions=false);

	KCodecAction(const QString &text, QObject *parent,bool showAutoOptions=false);

	KCodecAction(const KIcon &icon, const QString &text, QObject *parent,bool showAutoOptions=false);

	virtual ~KCodecAction();

public:
        int mibForName(const QString &codecName, bool *ok = 0) const;
        QTextCodec *codecForMib(int mib) const;

	QTextCodec *currentCodec() const;
	bool setCurrentCodec(QTextCodec *codec);

	QString currentCodecName() const;
	bool setCurrentCodec(const QString &codecName);

	int currentCodecMib() const;
	bool setCurrentCodec(int mib);

        /**
         * Applicable only if showAutoOptions in c'tor was true
         *
         * @returns KEncodingDetector::None if specific encoding is selected, not autodetection, otherwise... you know it!
         */
	KEncodingDetector::AutoDetectScript currentAutoDetectScript() const;
        /**
         * Applicable only if showAutoOptions in c'tor was true
         *
         * KEncodingDetector::SemiautomaticDetection means 'Default' item
         */
	bool setCurrentAutoDetectScript(KEncodingDetector::AutoDetectScript);


Q_SIGNALS:
        /**
         * Specific (proper) codec was selected
         */
	void triggered(QTextCodec *codec);

        /**
         * Specific (proper) codec was selected
         *
         * @returns codec name
         */
        void triggered(const QString&);

        /**
         * Autodetection has been selected.
         * emits KEncodingDetector::SemiautomaticDetection if Default was selected.
         *
         * Applicable only if showAutoOptions in c'tor was true
         */
        void triggered(KEncodingDetector::AutoDetectScript);

        /**
         * If showAutoOptions==true, then better handle triggered(KEncodingDetector::AutoDetectScript) signal
         */
        void defaultItemTriggered();


protected Q_SLOTS:
	virtual void actionTriggered(QAction*);

private:
	class Private;
	Private* const d;

    Q_PRIVATE_SLOT( d, void _k_subActionTriggered(QAction*) )
};

#endif
