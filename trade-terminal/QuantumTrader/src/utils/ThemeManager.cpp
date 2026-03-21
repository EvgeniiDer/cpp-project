#include"ThemeManager.h"
#include<QFile>
#include<QDebug>


ThemeManager& ThemeManager::instance()
{
	static ThemeManager _instance;
	return _instance;
}

ThemeManager::ThemeManager() : m_currentTheme(Theme::Dark)
{}

void ThemeManager::setTheme(Theme theme)
{
	if (theme == Theme::Custom)
	{
		qWarning() << QObject::tr("[ThemeManager] Please use setCustomTheme() for Custom themes!");
		return;
	}
	
	m_currentTheme = theme;
	QString path = (theme == Theme::Dark) ? ":/styles/dark_theme.qss" : ":/styles/light_theme.qss";

	QString styleSheet = loadFromFile(path);
	if (!styleSheet.isEmpty())
	{
		emit themeChanged(styleSheet);
	}
	else
	{
		qCritical() << QObject::tr("[ThemeManager] StyleSheet is empty or file not found:") << path;
		return;
	}
}
void ThemeManager::setCustomTheme(const QString& absoluteFilePath)
{
	m_currentTheme = Theme::Custom;
	QString styleSheet = loadFromFile(absoluteFilePath);
	if (!styleSheet.isEmpty())
	{
		emit themeChanged(styleSheet);
	}
	else
	{
		qCritical() << QObject::tr("[ThemeManager] absoluteFilePath is empty or file not found:") << absoluteFilePath;
		return;
	}
}
void ThemeManager::setRawStyle(const QString& rawCss)
{
	m_currentTheme = Theme::Custom;
	if (!rawCss.isEmpty())
	{
		emit themeChanged(rawCss);
	}
	else
	{
		qCritical() << QObject::tr("[ThemeManager] rawCss string is empty:") << rawCss;
		return;
	}
}
ThemeManager::Theme ThemeManager::currentTheme()const
{
	return m_currentTheme;
}
QString ThemeManager::loadFromFile(const QString& path)const
{
	QFile file(path);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		qCritical() << QObject::tr("[ThemeManager] Error opening theme file: ") << path;
		return "";
	}
	QString data = QLatin1String(file.readAll());
	file.close();
	return data;
}