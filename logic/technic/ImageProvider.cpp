#include "ImageProvider.h"
#include "logic/net/CacheDownload.h"
#include <QImage>
#include <QPixmap>
#include <QEventLoop>

#include "logger/QsLog.h"
#include "MultiMC.h"

ImageProvider::ImageProvider() :
    QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QImage ImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
	QLOG_ERROR() << "Image" << id;
	return QImage();
}

QPixmap ImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
	auto parts = id.split('$');
	if(parts.size() < 2 || parts.size() > 3)
	{
		return QPixmap();
	}

	// get the local cache path/identifier
	auto key = parts[0];

	// we cache stuff on our end, so we don't have to do HEAD requests all the time
	if(m_pixmaps.contains(key))
	{
		return m_pixmaps[key];
	}

	// decode url from base64
	QByteArray ba;
	ba.append(parts[1]);
	auto url = QByteArray::fromBase64(ba);


	// assume the images can change
	auto metaentry = MMC->metacache()->resolveEntry("cache", QLatin1Literal("technic/") + key);
	metaentry->stale = true;
	auto download = CacheDownload::make(QUrl(url), metaentry);

	// terrible workaround for lack of synchronous downloads... FIXME: replace.
	{
		bool failed = true;
		QEventLoop loop;
		auto goodCatcher = [&failed, &loop]()
		{
			loop.exit(1);
		};
		auto badCatcher = [&failed, &loop]()
		{
			loop.exit(0);
		};
		connect(download.get(), &NetAction::succeeded, goodCatcher);
		connect(download.get(), &NetAction::failed, badCatcher);
		download->start();
		if(!loop.exec())
		{
			QLOG_ERROR() << "Download failed for " << url;
			return QPixmap();
		}
	}

	QPixmap p;
	if(!p.load(download->getTargetFilepath()))
	{
		QLOG_ERROR() << "Load failed for " << key;
	}
	m_pixmaps[key] = p;
	return p;
}
