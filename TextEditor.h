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

// File Explorer için dosya türü
enum FileType
{
    FILE_TYPE_FOLDER,
    FILE_TYPE_TEXT,
    FILE_TYPE_CPP,
    FILE_TYPE_HEADER,
    FILE_TYPE_PYTHON,
    FILE_TYPE_JAVASCRIPT,
    FILE_TYPE_OTHER
};

// File Explorer item
struct FileItem
{
    std::string name;
    std::string fullPath;
    FileType type;
    bool isExpanded;
    int level;
    std::vector<FileItem> children;

    FileItem() : type(FILE_TYPE_OTHER), isExpanded(false), level(0) {}
};

// File Explorer panel
struct FileExplorer
{
    RECT rect;
    std::vector<FileItem> items;
    int scrollTop;
    int selectedIndex;
    std::string currentPath;

    FileExplorer() : scrollTop(0), selectedIndex(-1) {}
};

// Terminal panel
struct Terminal
{
    RECT rect;
    std::vector<std::string> output;
    std::string currentInput;
    int scrollTop;
    bool isActive;

    Terminal() : scrollTop(0), isActive(false) {}
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

    // New UI components
    FileExplorer fileExplorer;
    Terminal terminal;
    bool showFileExplorer;
    bool showTerminal;

    // Layout dimensions
    int fileExplorerWidth;
    int terminalHeight;

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
    HBRUSH explorer_brush;
    HBRUSH terminal_brush;

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

    // New methods for file explorer and terminal
    void initializeFileExplorer();
    void refreshFileExplorer();
    void drawFileExplorer(HDC hdc);
    void handleFileExplorerClick(int x, int y);
    void loadDirectory(const std::string &path, std::vector<FileItem> &items, int level);
    FileType getFileType(const std::string &filename);

    void drawTerminal(HDC hdc);
    void handleTerminalInput(char ch);
    void executeTerminalCommand(const std::string &command);
    void addTerminalOutput(const std::string &text);
    void handleTerminalClick(int x, int y);

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