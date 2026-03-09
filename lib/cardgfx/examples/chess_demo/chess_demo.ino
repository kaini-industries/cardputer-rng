/**
 * Meshtastic Chess — CardGFX Example
 *
 * Demonstrates how the full chess game maps onto the CardGFX library.
 * This is a local two-player (pass & play) skeleton.
 * Add mesh_comm layer for networked play.
 */

#include <cardgfx.h>

using namespace CardGFX;

// ── Chess Piece Sprites (5x7 1-bit bitmaps) ─────────────────────
// Simple iconic representations at 5x7 pixels each.
// These get scaled up to 10x14 or drawn into 16x16 cells.

// Piece IDs
enum Piece : uint8_t {
    EMPTY  = 0,
    W_KING = 1, W_QUEEN, W_ROOK, W_BISHOP, W_KNIGHT, W_PAWN,
    B_KING = 7, B_QUEEN, B_ROOK, B_BISHOP, B_KNIGHT, B_PAWN,
};

static bool isWhitePiece(uint8_t p) { return p >= W_KING && p <= W_PAWN; }
static bool isBlackPiece(uint8_t p) { return p >= B_KING && p <= B_PAWN; }
static bool isPiece(uint8_t p)      { return p != EMPTY; }

// Piece characters for text rendering
static const char* pieceChar(uint8_t p) {
    switch (p) {
        case W_KING: case B_KING:     return "K";
        case W_QUEEN: case B_QUEEN:   return "Q";
        case W_ROOK: case B_ROOK:     return "R";
        case W_BISHOP: case B_BISHOP: return "B";
        case W_KNIGHT: case B_KNIGHT: return "N";
        case W_PAWN: case B_PAWN:     return "P";
        default: return "";
    }
}

// ── Board State ──────────────────────────────────────────────────

struct ChessBoard {
    uint8_t squares[8][8];       // [row][col], row 0 = rank 8 (top)
    bool    whiteToMove;
    bool    wKingCastle, wQueenCastle;
    bool    bKingCastle, bQueenCastle;
    int8_t  enPassantCol;        // -1 = none
    uint16_t moveNumber;

    void reset() {
        const uint8_t backRank[] = {
            W_ROOK, W_KNIGHT, W_BISHOP, W_QUEEN,
            W_KING, W_BISHOP, W_KNIGHT, W_ROOK
        };
        memset(squares, EMPTY, sizeof(squares));

        for (int c = 0; c < 8; c++) {
            squares[0][c] = backRank[c] + 6;  // Black back rank
            squares[1][c] = B_PAWN;
            squares[6][c] = W_PAWN;
            squares[7][c] = backRank[c];       // White back rank
        }

        whiteToMove = true;
        wKingCastle = wQueenCastle = true;
        bKingCastle = bQueenCastle = true;
        enPassantCol = -1;
        moveNumber = 1;
    }
};

// ── Game State ───────────────────────────────────────────────────

struct GameState {
    ChessBoard board;
    int8_t selectedCol = -1;   // Piece picked up (-1 = none)
    int8_t selectedRow = -1;
    char   moveHistory[32][8]; // Algebraic notation history
    uint8_t moveCount = 0;
    char   statusText[32] = "White to move";

    void init() {
        board.reset();
        selectedCol = selectedRow = -1;
        moveCount = 0;
        strcpy(statusText, "White to move");
    }
};

static GameState g_game;

// ── Cell Renderer ────────────────────────────────────────────────
// This callback draws each cell of the Grid widget.

void renderChessCell(Canvas& canvas, uint8_t col, uint8_t row,
                     int16_t cx, int16_t cy, uint8_t cellW, uint8_t cellH,
                     Grid::CellState state, const Theme& theme,
                     void* context) {
    // Cell background (checkerboard)
    bool lightSquare = ((col + row) % 2 == 0);
    uint16_t bg = lightSquare ? theme.gridCellA : theme.gridCellB;

    // State overlays
    if (state.marked)    bg = theme.accentMuted;   // Last move
    if (state.highlight) bg = theme.gridHighlight;  // Valid move target
    if (state.selected)  bg = theme.accentActive;   // Picked-up piece

    canvas.fillRect(cx, cy, cellW, cellH, bg);

    // Piece
    uint8_t piece = g_game.board.squares[row][col];
    if (piece != EMPTY) {
        uint16_t pieceColor = isWhitePiece(piece)
                              ? HAL::rgb565(255, 255, 255)
                              : HAL::rgb565(30, 30, 30);
        // Draw piece letter centered in cell
        const char* ch = pieceChar(piece);
        uint8_t scale = 1;
        uint16_t tw = canvas.textWidth(ch, scale);
        int16_t tx = cx + (cellW - tw) / 2;
        int16_t ty = cy + (cellH - 7) / 2;

        // Outline for contrast on both light/dark squares
        uint16_t outline = isWhitePiece(piece)
                           ? HAL::rgb565(0, 0, 0)
                           : HAL::rgb565(200, 200, 200);
        canvas.drawText(tx - 1, ty, ch, outline, scale);
        canvas.drawText(tx + 1, ty, ch, outline, scale);
        canvas.drawText(tx, ty - 1, ch, outline, scale);
        canvas.drawText(tx, ty + 1, ch, outline, scale);
        canvas.drawText(tx, ty, ch, pieceColor, scale);
    }

    // Cursor
    if (state.cursor) {
        canvas.drawRect(cx, cy, cellW, cellH, theme.gridCursor);
        if (cellW > 4) {
            canvas.drawRect(cx + 1, cy + 1,
                            cellW - 2, cellH - 2, theme.gridCursor);
        }
    }
}

// ── Scene: Game ──────────────────────────────────────────────────

class GameScene : public Scene {
public:
    GameScene() : Scene("game") {}

    // Widgets
    StatusBar topBar;
    Grid      boardGrid;
    List      moveList;
    Label     turnLabel;
    Label     statusLabel;
    TextInput moveInput;

    void setup() {
        // ── Top status bar ───────────────────────────────────────
        topBar.setId(1);
        topBar.setBounds({0, 0, SCREEN_W, 11});
        topBar.setLeft("MeshChess");
        topBar.setCenter("Local");
        topBar.setRight("v0.1");
        addWidget(&topBar);

        // ── Chess board (left side) ──────────────────────────────
        boardGrid.setId(2);
        boardGrid.setGridSize(8, 8);
        boardGrid.setCellSize(15, 15);
        boardGrid.setBounds({2, 13, 120, 120});
        boardGrid.setCellRenderer(renderChessCell);
        boardGrid.setContext(&g_game);
        boardGrid.setCursor(4, 6);  // Start on e2
        boardGrid.setFocusable(true);
        boardGrid.setOnAction([this](uint8_t col, uint8_t row) {
            handleCellAction(col, row);
        });
        addWidget(&boardGrid, true);  // true = add to focus chain

        // ── Sidebar: Turn indicator ──────────────────────────────
        turnLabel.setId(3);
        turnLabel.setBounds({124, 13, 114, 11});
        turnLabel.setText("White");
        turnLabel.setAlign(Label::Align::Center);
        addWidget(&turnLabel);

        // ── Sidebar: Move history ────────────────────────────────
        moveList.setId(4);
        moveList.setBounds({124, 26, 114, 72});
        moveList.setShowIndex(true);
        moveList.setItemHeight(10);
        moveList.setAutoScroll(true);
        moveList.setFocusable(true);
        addWidget(&moveList, true);

        // ── Sidebar: Status text ─────────────────────────────────
        statusLabel.setId(5);
        statusLabel.setBounds({124, 100, 114, 11});
        statusLabel.setText("Ready");
        statusLabel.setAlign(Label::Align::Center);
        statusLabel.setColor(HAL::rgb565(46, 204, 113));
        addWidget(&statusLabel);

        // ── Move text input (bottom) ─────────────────────────────
        moveInput.setId(6);
        moveInput.setBounds({124, 113, 114, 14});
        moveInput.setPlaceholder("e2e4");
        moveInput.setMaxLength(6);
        moveInput.setFocusable(true);
        moveInput.setOnSubmit([this](const char* text) {
            handleTextMove(text);
        });
        addWidget(&moveInput, true);
    }

    void onEnter() override {
        g_game.init();
        updateUI();
    }

    void onTick(uint32_t dt_ms) override {
        // Could animate the cursor, check network, etc.
    }

private:
    void handleCellAction(uint8_t col, uint8_t row) {
        uint8_t piece = g_game.board.squares[row][col];

        if (g_game.selectedCol < 0) {
            // No piece selected — pick up a piece
            if (piece == EMPTY) return;
            if (g_game.board.whiteToMove && !isWhitePiece(piece)) return;
            if (!g_game.board.whiteToMove && !isBlackPiece(piece)) return;

            g_game.selectedCol = col;
            g_game.selectedRow = row;
            boardGrid.setSelected(col, row, true);

            // TODO: Highlight valid move targets
            // For now, just mark the selected square
        } else {
            // Piece already selected — attempt move
            uint8_t fromCol = g_game.selectedCol;
            uint8_t fromRow = g_game.selectedRow;

            // Clear selection visuals
            boardGrid.clearAllSelected();
            boardGrid.clearAllHighlights();
            boardGrid.clearAllMarks();

            if (col == fromCol && row == fromRow) {
                // Deselect
                g_game.selectedCol = g_game.selectedRow = -1;
                return;
            }

            // Execute move (simplified — no validation yet)
            g_game.board.squares[row][col] =
                g_game.board.squares[fromRow][fromCol];
            g_game.board.squares[fromRow][fromCol] = EMPTY;

            // Mark last move
            boardGrid.setMarked(fromCol, fromRow, true);
            boardGrid.setMarked(col, row, true);

            // Record move
            char move[8];
            snprintf(move, sizeof(move), "%c%c%c%c",
                     'a' + fromCol, '8' - fromRow,
                     'a' + col, '8' - row);
            moveList.addItem(move);
            moveList.scrollToBottom();

            // Switch turn
            g_game.board.whiteToMove = !g_game.board.whiteToMove;
            g_game.selectedCol = g_game.selectedRow = -1;
            g_game.board.moveNumber++;

            updateUI();
        }
    }

    void handleTextMove(const char* text) {
        // Parse algebraic notation like "e2e4"
        if (strlen(text) < 4) return;

        uint8_t fromCol = text[0] - 'a';
        uint8_t fromRow = '8' - text[1];
        uint8_t toCol   = text[2] - 'a';
        uint8_t toRow   = '8' - text[3];

        if (fromCol > 7 || fromRow > 7 || toCol > 7 || toRow > 7) return;

        // Set cursor to destination and simulate action
        boardGrid.setCursor(fromCol, fromRow);
        handleCellAction(fromCol, fromRow);  // Select
        handleCellAction(toCol, toRow);      // Move

        moveInput.clear();
    }

    void updateUI() {
        turnLabel.setText(g_game.board.whiteToMove ? "White" : "Black");

        char status[32];
        snprintf(status, sizeof(status), "Move %d",
                 g_game.board.moveNumber);
        statusLabel.setText(status);
    }
};

// ── Main ─────────────────────────────────────────────────────────

static GameScene gameScene;

void setup() {
    // Initialize CardGFX (display, input, framebuffer)
    CardGFX::init(1, 128);

    // Setup and register the game scene
    gameScene.setup();
    CardGFX::scenes().registerScene(&gameScene);

    // Push the game scene as the initial screen
    CardGFX::scenes().push(&gameScene);
}

void loop() {
    // That's it. CardGFX handles everything.
    CardGFX::tick();
}
