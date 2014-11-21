#include "QMLDialog.h"
#include <QQuickView>
#include <QHBoxLayout>
#include <QQmlContext>
#include <QQuickItem>

QMLDialog::QMLDialog(QUrl sourceUrl, QWidget * parent)
    : QDialog(parent)
{
	m_view = new QQuickView(this->windowHandle());
	m_view->rootContext()->setContextProperty("dialog",this);
	m_view->setSource(sourceUrl);

	int minWidth = getPropertyValue("minWidth", -1).toInt();
	int minHeight = getPropertyValue("minHeight", -1).toInt();
	setMinimumSize(QSize(minWidth, minHeight));

	auto layout = new QHBoxLayout();
	layout->setMargin(0);
	QWidget *container = QWidget::createWindowContainer(m_view);
	container->setParent(this);
	layout->addWidget(container);
	setLayout(layout);
	m_view->setResizeMode(QQuickView::SizeRootObjectToView);
}

QMLDialog::~QMLDialog()
{
	delete m_view;
}

QVariant QMLDialog::getPropertyValue(const char* name, QVariant defVal)
{
	auto rootobj = m_view->rootObject();
	auto propertyObj = rootobj->property(name);
	if(!propertyObj.isValid())
	{
		return defVal;
	}
	if(defVal.type() != propertyObj.type())
	{
		return defVal;
	}
	return propertyObj;
}

void QMLDialog::startDragging()
{
    m_dragPoint = mapFromGlobal(QCursor::pos());
}

void QMLDialog::updateDragging()
{
    QPoint draggingNow = QCursor::pos();
    QPoint pos = draggingNow - m_dragPoint;
    move(pos);
}
