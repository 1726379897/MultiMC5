#include "PackModel.h"
#include <QUrl>
#include <QStringListModel>
#include <QPixmap>
#include "logic/MMCJson.h"
#include "logic/net/ByteArrayDownload.h"
#include "logger/QsLog.h"
#include <modutils.h>

/*
	"tekkitmain":
	{
		"name": "tekkitmain",
		"display_name": "Tekkit",
		"url": "http:\/\/www.technicpack.net\/tekkit",
		"icon": "http:\/\/cdn.technicpack.net\/resources\/tekkitmain\/icon.png?1410213214",
		"icon_md5": "1dd87c03268a7144411bb8cbe8bf7326",
		"logo": "http:\/\/cdn.technicpack.net\/resources\/tekkitmain\/logo.png?1410213214",
		"logo_md5": "2f9625f8343cd1aaa35d3dd631ad64e1",
		"background": "http:\/\/cdn.technicpack.net\/resources\/tekkitmain\/background.png?1410213214",
		"background_md5": "f39ae618809383451f6832e4d2a738fe",
		"recommended": "1.2.9e",
		"latest": "1.2.10c",
		"builds": [
			"1.0.2",
			"1.0.3",
			...
		]
	},
*/

std::shared_ptr<SolderPackInfo> loadSolderPackInfo(QJsonObject object)
{
	using namespace MMCJson;
	auto packInfo = std::make_shared<SolderPackInfo>();
	try
	{
		auto remoteImage = [&packInfo, &object](QString ident) -> QString
		{
			QString value = object.value(ident).toString();
			if(value.isEmpty())
			{
				return value;
			}
			QByteArray ba;
			ba.append(value);
			auto result = QString("image://url/%1/%2$%3").arg(packInfo->name, ident, ba.toBase64());
			QString md5 = object.value(ident + "_md5").toString();
			if(!md5.isEmpty())
			{
				result.append('$');
				result.append(md5);
			}
			return result;
		};

		// TODO: more than one repo/pack subscription...
		packInfo->repo = "http://solder.technicpack.net/api/modpack/";
		packInfo->name = ensureString(object.value("name"), "name");
		packInfo->display_name = ensureString(object.value("display_name"), "display_name");
		packInfo->url = object.value("url").toString();
		packInfo->icon = remoteImage("icon");
		packInfo->logo = remoteImage("logo");
		packInfo->background = remoteImage("background");
		QLOG_INFO() << "background:" << packInfo->background;
		QStringList builds =  ensureStringList(object.value("builds"), "builds");

		// sort builds
		auto versionCompare = [](QString first, QString second)
		{
			if(first.startsWith('v'))
			{
				first.remove(0,1);
			}
			if(second.startsWith('v'))
			{
				second.remove(0,1);
			}
			Util::Version left(first);
			Util::Version right(second);
			auto rightLess = left > right;
			if(rightLess)
			{
				QLOG_DEBUG() << right.toString() << "<" << left.toString();
			}
			else
			{
				QLOG_DEBUG() << left.toString() << "<" << right.toString();
			}
			return rightLess;
		};
		std::sort(builds.begin(), builds.end(), versionCompare);

		packInfo->builds = builds;
		QString recommended = ensureString(object.value("recommended"), "recommended");
		packInfo->recommended = packInfo->builds.indexOf(recommended);
		QString latest = ensureString(object.value("latest"), "latest");
		packInfo->latest = packInfo->builds.indexOf(latest);
	}
	catch (JSONValidationError & e)
	{
		QLOG_ERROR() << "Error parsing Solder pack: " << e.cause();
		return nullptr;
	}
	return packInfo;
}


PackModel::PackModel(QObject* parent) : QMLAbstractListModel(parent)
{
	populate();
}

void PackModel::populate()
{
	QString source("http://solder.technicpack.net/api/modpack/?include=full");

	m_dlAction = ByteArrayDownload::make(QUrl(source));
	connect(m_dlAction.get(), SIGNAL(succeeded(int)), SLOT(dataAvailable()));
	m_dlAction->start();
}

SolderPackInfoPtr PackModel::packByIndex(int index)
{
	if(index < 0 || index >= rowCount())
		return nullptr;

	return m_packs[index];
}

void PackModel::dataAvailable()
{
	beginResetModel();
	m_packs.clear();
	auto document = QJsonDocument::fromJson(m_dlAction->m_data);
	if(document.isNull())
	{
		QLOG_ERROR() << m_dlAction->m_data;
		QLOG_ERROR() << "Got gibberish from Technic instead of a pack list";
		return;
	}
	auto modpacksValue = document.object().value("modpacks");
	if(modpacksValue.isNull())
	{
		QLOG_ERROR() << "No modpacks in the retrieved json";
		return;
	}
	auto modpacksObject = modpacksValue.toObject();
	auto iter = modpacksObject.begin();
	while (iter != modpacksObject.end())
	{
		QString packName = iter.key();
		auto packValue = iter.value();
		if(!packValue.isObject())
		{
			QLOG_ERROR() << "Pack" << packName << "is not an object.";
			iter++;
			continue;
		}
		auto pack = loadSolderPackInfo(packValue.toObject());
		if(!pack)
		{
			QLOG_ERROR() << "Pack" << packName << "could not be loaded.";
			iter++;
			continue;
		}
		m_packs.push_back(pack);
		iter++;
	}
	endResetModel();
}

QHash<int, QByteArray> PackModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NameRole] = "name";
	roles[DisplayNameRole] = "display_name";
	roles[LogoRole] = "logo";
	roles[BackgroundRole] = "background";
	roles[RecommendedRole] = "recommended";
	roles[LatestRole] = "latest";
	return roles;
}

QVariant PackModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid() || index.row() < 0 || index.row() >= rowCount())
		return QVariant();

	auto pack = m_packs[index.row()];
	switch(role)
	{
		case Qt::DisplayRole:
		case NameRole:
			return  pack->name;
		case DisplayNameRole:
			return  pack->display_name;
		case LogoRole:
			return pack->logo;
		case BackgroundRole:
			return pack->background;
		case RecommendedRole:
			return pack->recommended;
		case LatestRole:
			return pack->latest;
		default:
			return QVariant();
	}
}

QVariantMap QMLAbstractListModel::get(int row)
{
	QHash<int,QByteArray> names = roleNames();
	QHashIterator<int, QByteArray> i(names);
	QVariantMap res;
	while (i.hasNext())
	{
		i.next();
		QModelIndex idx = index(row, 0);
		QVariant data = idx.data(i.key());
		res[i.value()] = data;
	}
	return res;
}

int PackModel::rowCount(const QModelIndex&) const
{
	return m_packs.size();
}

QHash<int, QByteArray> VersionModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[NameRole] = "name";
	roles[RecommendedRole] = "recommended";
	roles[LatestRole] = "latest";
	return roles;
}

QVariant VersionModel::data(const QModelIndex& index, int role) const
{
	if(!m_base)
	{
		return QVariant();
	}
	auto strings = m_base->builds;

	if(!index.isValid() || index.row() < 0 || index.row() >= strings.size())
		return QVariant();

	auto row = index.row();
	auto string = strings[row];
	switch(role)
	{
		case Qt::DisplayRole:
		case NameRole:
			return string;
		case LatestRole:
			return row == m_base->latest;
		case RecommendedRole:
			return row == m_base->recommended;
		default:
			return QVariant();
	}
}

int VersionModel::rowCount(const QModelIndex&) const
{
	if(!m_base)
	{
		return 0;
	}
	return m_base->builds.size();
}
