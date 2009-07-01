/* (C)opyright 2009 Anton Novikov
 * See LICENSE file for license details.
 *
 * qtimespinbox.h
 */

#ifndef H_QTIMESPINBOX
#define H_QTIMESPINBOX

#include <QDoubleSpinBox>
#include <QValidator>

class QTimeSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

	QRegExpValidator validator;
public:
	explicit QTimeSpinBox(QWidget *dlg = NULL);
	QString textFromValue(double) const;
	double valueFromText(const QString &) const;
	QValidator::State validate(QString &input, int &pos) const;
	void fixup(QString &) const;

public slots:
	void setMinimum(double v) { QDoubleSpinBox::setMinimum(v); }
	void setMaximum(double v) { QDoubleSpinBox::setMaximum(v); }
};

#endif /* H_QTIMESPINBOX */
