#include <QStringList>
#include "qtimespinbox.h"
#include "util.h"

QTimeSpinBox::QTimeSpinBox(QWidget *parent)
	: QDoubleSpinBox(parent),
	validator(QRegExp(QString("[0-9]*(:[0-9]{0,2}){0,2}(\\.[0-9]{0,3})?")), this)
{
	setDecimals(3);
}

QString QTimeSpinBox::textFromValue(double value) const
{
	return time2string(value, decimals());
}

double QTimeSpinBox::valueFromText(const QString &input) const
{
	QStringList sl = input.split(":");

	double ret = 0;
	for (QStringList::iterator i = sl.begin(); i != sl.end(); ++i)
		ret = ret * 60 + i->toDouble();
	return ret;
}

QValidator::State QTimeSpinBox::validate(QString &input, int &pos) const
{
	return validator.validate(input, pos);
}

void QTimeSpinBox::fixup(QString &input) const
{
	return validator.fixup(input);
}

