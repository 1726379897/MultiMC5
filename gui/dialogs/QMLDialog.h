#pragma once
#include <QDialog>
#include <QUrl>
#include <QVariant>

class QQuickView;

class QMLDialog: public QDialog
{
	Q_OBJECT
public:
	explicit QMLDialog(QUrl sourceUrl, QWidget * parent = 0);
	virtual ~QMLDialog();

public slots:
	void startDragging();
	void updateDragging();

public:
	QVariant getPropertyValue (const char * name, QVariant defVal);

protected: /* variables */
    QPoint m_dragPoint;
	QQuickView *m_view;
};
