#pragma once

class WindowManager;

using ActionFactory = std::function<QAction* (WindowManager*, QObject*)>;


class ActionManager : public QObject
{
	Q_OBJECT
public:
	explicit ActionManager(WindowManager* windowManager, QObject* parent = nullptr);
	void registerActionFactory(const QString& id, ActionFactory factory);
	QAction* getAction(const QString& id);
private:
	WindowManager* m_windowManager;
	QMap<QString, ActionFactory> m_factories;
	QMap<QString, QAction*> m_actions;

	void initRegistration();
	
};