/**
 * @file
 */

#pragma once

#include "FileDialog.h"
#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "RenderShaders.h"
#include "Console.h"
#include "core/collection/Array.h"

struct SDL_Cursor;

namespace ui {
namespace imgui {

// https://github.com/aiekick/ImGuiFileDialog
/**
 * @ingroup UI
 */
class IMGUIApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	video::Camera _camera;
	void loadFonts();
protected:
	core::VarPtr _renderUI;
	core::VarPtr _showMetrics;
	core::VarPtr _uiFontSize;
	video::Id _texture = video::InvalidId;
	shader::TextureShader _shader;
	video::Buffer _vbo;
	Console _console;
	int32_t _bufferIndex = -1;
	int32_t _indexBufferIndex = -1;
	int8_t _mouseWheelX = 0;
	int8_t _mouseWheelY = 0;
	bool _mousePressed[3] = {false};
	core::String _writePathIni;
	core::String _writePathLog;
	core::VarPtr _lastDirectory;

	bool _showBindingsDialog = false;
	bool _showFileDialog = false;
	bool _persistUISettings = true;

	SDL_Cursor* _mouseCursors[ImGuiMouseCursor_COUNT];
	OpenFileMode _fileDialogMode = OpenFileMode::Directory;
	std::function<void(const core::String&)> _fileDialogCallback {};

	ImFont* _defaultFont = nullptr;
	ImFont* _bigFont = nullptr;
	ImFont* _smallFont = nullptr;

	FileDialog _fileDialog;

	void executeDrawCommands();

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
public:
	IMGUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~IMGUIApp();

	const video::Camera& camera() const;

	virtual void beforeUI();

	int fontSize() const;
	virtual void onWindowClose(void *windowHandle) override;
	virtual void onWindowMoved(void *windowHandle) override;
	virtual void onWindowFocusGained(void *windowHandle) override;
	virtual void onWindowFocusLost(void *windowHandle) override;
	virtual void onWindowResize(void *windowHandle, int windowWidth, int windowHeight) override;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual void onRenderUI() = 0;
	virtual app::AppState onCleanup() override;

	ImFont *defaultFont();
	ImFont *bigFont();
	ImFont *smallFont();

	void showBindingsDialog();
	void fileDialog(const std::function<void(const core::String&)>& callback, OpenFileMode mode, const io::FormatDescription* formats = nullptr) override;
};

inline void IMGUIApp::showBindingsDialog() {
	_showBindingsDialog = true;
}

inline ImFont *IMGUIApp::defaultFont() {
	return _defaultFont;
}

inline ImFont *IMGUIApp::bigFont() {
	return _bigFont;
}

inline ImFont *IMGUIApp::smallFont() {
	return _smallFont;
}

inline int IMGUIApp::fontSize() const {
	return _uiFontSize->intVal();
}

inline const video::Camera& IMGUIApp::camera() const {
	return _camera;
}

}
}

inline ui::imgui::IMGUIApp* imguiApp() {
	return (ui::imgui::IMGUIApp*)video::WindowedApp::getInstance();
}
