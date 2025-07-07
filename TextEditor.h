#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <cctype>
#include <richedit.h>
#include <commdlg.h>
#include <algorithm>
#include "EditorPane.h"

// Editör modları
enum EditorMode
{
    INSERT_MODE,
    COMMAND_MODE
};

enum SplitDirection
{
    VERTICAL_SPLIT,
    HORIZONTAL_SPLIT
};

// Ana editör sınıfı
class ModernTextEditor
{
private:
    HWND hwnd;
    EditorMode mode;
    std::vector<EditorPane> panes;
    int active_pane;
    SplitDirection split_direction;
    bool shift_pressed;
    bool ctrl_pressed;
    std::string command_buffer;
    std::string status_message;
    std::string clipboard_data;

    // Search & Replace
    bool search_mode;
    bool replace_mode;
    std::string search_text;
    std::string replace_text;
    int current_search_result;

    // Undo/Redo system
    struct UndoState
    {
        std::vector<std::string> lines;
        int cursor_row;
        int cursor_col;
        std::string operation;
    };

    std::vector<UndoState> undo_stack;
    std::vector<UndoState> redo_stack;
    int max_undo_levels;

    // Font ayarları
    HFONT hFont;
    int char_width;
    int char_height;

    // Renkler
    HBRUSH bg_brush;
    HBRUSH status_brush;
    HBRUSH cursor_brush;
    HBRUSH selection_brush;

public:
    ModernTextEditor();
    ~ModernTextEditor();

    void setHwnd(HWND h);
    void calculateCharSize();
    void updatePaneLayout();
    void handleKeyPress(WPARAM wParam);
    void handleInsertMode(WPARAM wParam);
    void handleCommandMode(WPARAM wParam);
    void handleChar(WPARAM wParam);
    void insertText(const std::string &text);
    void updateSelection();
    void selectAll();
    std::string getSelectedText();
    void copySelection();
    void cutSelection();
    void deleteSelection();
    void copyToClipboard(const std::string &text);
    void pasteFromClipboard();
    void insertMultilineText(const std::string &text);
    void executeCommand();
    void verticalSplit();
    void horizontalSplit();
    void closePane();
    void switchActivePane();
    void updatePanes();
    void switchToPane(int pane_index);
    void newFile();
    void openFile();
    void loadFile(const std::string &filename);
    void saveFile();
    void paint(HDC hdc);
    void drawPane(HDC hdc, const EditorPane &pane, int pane_index);
    void drawSelection(HDC hdc, const EditorPane &pane, int current_line, int line_y);
    void drawStatusBar(HDC hdc);
    void handleResize();
    void handleMouseClick(int x, int y);

    // Search & Replace functions
    void startSearch();
    void startReplace();
    void performSearch();
    void performReplace();

    // Undo/Redo functions
    void saveUndoState(const std::string &operation);
    void performUndo();
    void performRedo();
};