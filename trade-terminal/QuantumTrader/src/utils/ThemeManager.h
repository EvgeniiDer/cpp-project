#pragma once
#include<QObject>
#include<QString>
#include<QFile>
#include<QDebug>


class ThemeManager : public QObject
{
	Q_OBJECT
public:
	enum class Theme{Dark, Light, Custom};
	static ThemeManager& instance();

	ThemeManager(const ThemeManager&) = delete;
	ThemeManager& operator=(const ThemeManager&) = delete;

	void setTheme(Theme theme);
	void setCustomTheme(const QString& absoluteFilePath);
	void setRawStyle(const QString& rawCss);

	Theme currentTheme()const;

signals:
	void themeChanged(const QString& styleSheet);
	
private:
	ThemeManager();
	QString loadFromFile(const QString& path)const;
	Theme m_currentTheme;
};
