#pragma once
#include "esphome.h"

namespace tetris {

// Board: 8 wide × 16 tall.
// Convención lógica: y=0 ARRIBA (donde spawn la pieza), y=15 ABAJO (donde se acumula).
// El render hace flip vertical para mapear al strip (donde y=0 está físicamente abajo).
constexpr int W = 8;
constexpr int H = 16;

// Tetrominoes — bitmap 4×4 codificado en uint16_t.
// MSB es top-left. Lectura: bit (15 - r*4 - c) corresponde a (row=r, col=c).
// 4 rotaciones por pieza, aunque algunas son idénticas (S/Z/I se repiten).
constexpr uint16_t PIECES[7][4] = {
    // 0: I
    {0x0F00, 0x2222, 0x00F0, 0x4444},
    // 1: O
    {0x6600, 0x6600, 0x6600, 0x6600},
    // 2: T
    {0x4E00, 0x4640, 0x0E40, 0x4C40},
    // 3: S
    {0x6C00, 0x8C40, 0x06C0, 0x08C4},
    // 4: Z
    {0xC600, 0x4C80, 0x0C60, 0x04C8},
    // 5: J
    {0x8E00, 0x6440, 0x0E20, 0x44C0},
    // 6: L
    {0x2E00, 0x4460, 0x0E80, 0xC440},
};

// Colores por tipo de pieza. Valores moderados (~100/255) para que con
// brightness al 100% no se pase de corriente. Sin gamma_correct (1.0)
// los valores se transmiten literales al strip.
constexpr esphome::Color PIECE_COLORS[7] = {
    esphome::Color(  0, 100, 100),  // I — cyan
    esphome::Color(100, 100,   0),  // O — yellow
    esphome::Color( 80,   0, 100),  // T — purple
    esphome::Color(  0, 100,   0),  // S — green
    esphome::Color(100,   0,   0),  // Z — red
    esphome::Color(  0,   0, 120),  // J — blue
    esphome::Color(110,  50,   0),  // L — orange
};

// Mapping físico: (x, y_phys) lógico-físico → índice en el strip de 128 LEDs.
// y_phys=0 abajo, y_phys=15 arriba. Serpentine row-major: filas pares
// izq→der, filas impares der→izq.
inline int physical_index(int x, int y_phys) {
  if (y_phys & 1) return y_phys * W + (W - 1 - x);
  else            return y_phys * W + x;
}

enum GameState : uint8_t { IDLE, PLAYING, GAME_OVER };

struct Piece {
  uint8_t type;      // 0-6
  uint8_t rotation;  // 0-3
  int8_t  x, y;      // posición top-left del bitmap 4×4 en coords del board
};

class TetrisGame {
 public:
  // Estado expuesto a los sensors HA.
  GameState state = IDLE;
  uint16_t  score = 0;
  uint16_t  lines = 0;

  TetrisGame() {
    clear_board();
    refill_bag();
  }

  void new_game() {
    clear_board();
    refill_bag();
    score = 0;
    lines = 0;
    state = PLAYING;
    spawn();
  }

  // Llamado cada 800ms desde el interval del YAML.
  void tick() {
    if (state != PLAYING) return;
    if (!try_move(0, 1)) {
      // No pudo bajar → lockear pieza y spawnear siguiente.
      lock_piece();
      int cleared = clear_lines();
      lines += cleared;
      score += cleared * 10;
      spawn();
    }
  }

  void move_left()  { if (state == PLAYING) try_move(-1, 0); }
  void move_right() { if (state == PLAYING) try_move( 1, 0); }

  void rotate() {
    if (state != PLAYING) return;
    uint8_t prev_rot = active.rotation;
    int8_t  prev_x   = active.x;
    int8_t  prev_y   = active.y;
    active.rotation = (active.rotation + 1) & 0x03;

    // SRS-lite: si la rotación choca, probamos un set de offsets en orden.
    // Captura los casos comunes (pared izq/der/abajo) sin la complejidad
    // del SRS oficial. Para la pieza I que es 4-wide hacen falta kicks
    // de 2 cells, por eso ±2 está incluido.
    static constexpr int8_t KICKS[][2] = {
        { 0,  0},  // sin kick (caso normal)
        {-1,  0}, { 1,  0},  // pared izq / der
        {-2,  0}, { 2,  0},  // pared izq / der para pieza I
        { 0, -1},            // floor kick (subir 1)
        {-1, -1}, { 1, -1},  // diagonales arriba
    };

    for (auto &k : KICKS) {
      active.x = prev_x + k[0];
      active.y = prev_y + k[1];
      if (!collides(active)) return;  // este kick funcionó
    }

    // Ningún kick funcionó: deshacer todo.
    active.rotation = prev_rot;
    active.x = prev_x;
    active.y = prev_y;
  }

  void hard_drop() {
    if (state != PLAYING) return;
    while (try_move(0, 1)) { /* slam down */ }
    // Forzar lock inmediato (en vez de esperar al siguiente tick).
    lock_piece();
    int cleared = clear_lines();
    lines += cleared;
    score += cleared * 10;
    spawn();
  }

  // Pinta el estado actual al strip. Se llama desde el effect "Game".
  void render(esphome::light::AddressableLight &it) {
    it.all() = esphome::Color::BLACK;

    // Piezas lockeadas.
    for (int y = 0; y < H; y++) {
      for (int x = 0; x < W; x++) {
        uint8_t c = board[y][x];
        if (c != 0) {
          // y lógico (0=arriba) → y_phys (0=abajo): flip vertical.
          it[physical_index(x, H - 1 - y)] = PIECE_COLORS[c - 1];
        }
      }
    }

    // Pieza activa.
    if (state == PLAYING) {
      uint16_t bits = PIECES[active.type][active.rotation];
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          if (bits & (1 << (15 - r * 4 - c))) {
            int bx = active.x + c;
            int by = active.y + r;
            if (bx >= 0 && bx < W && by >= 0 && by < H) {
              it[physical_index(bx, H - 1 - by)] = PIECE_COLORS[active.type];
            }
          }
        }
      }
    }

    // Game over: parpadeo rojo del board entero.
    if (state == GAME_OVER) {
      blink_counter_++;
      if ((blink_counter_ / 8) & 1) {
        for (int y = 0; y < H; y++)
          for (int x = 0; x < W; x++)
            it[physical_index(x, H - 1 - y)] = esphome::Color(80, 0, 0);
      }
    }

    // Idle: una "T" gris en el centro como pantalla de bienvenida.
    if (state == IDLE) {
      // Barra horizontal arriba (y=2 lógico, x=2..5).
      for (int x = 2; x < 6; x++)
        it[physical_index(x, H - 1 - 2)] = esphome::Color(30, 30, 30);
      // Stem vertical (x=3, y=2..7).
      for (int y = 2; y <= 7; y++)
        it[physical_index(3, H - 1 - y)] = esphome::Color(30, 30, 30);
    }
  }

 private:
  uint8_t board[H][W] = {};
  Piece   active{};
  uint8_t bag_[7] = {0, 1, 2, 3, 4, 5, 6};
  uint8_t bag_idx_ = 7;  // forzar refill en el primer next_piece
  uint16_t blink_counter_ = 0;

  void clear_board() {
    for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++)
        board[y][x] = 0;
  }

  // 7-bag: garantiza que cada pieza aparezca una vez cada 7 spawns.
  // Mejor distribución que random() puro.
  void refill_bag() {
    for (int i = 0; i < 7; i++) bag_[i] = i;
    for (int i = 6; i > 0; i--) {
      int j = esphome::random_uint32() % (i + 1);
      uint8_t t = bag_[i]; bag_[i] = bag_[j]; bag_[j] = t;
    }
    bag_idx_ = 0;
  }

  uint8_t next_type() {
    if (bag_idx_ >= 7) refill_bag();
    return bag_[bag_idx_++];
  }

  void spawn() {
    active.type = next_type();
    active.rotation = 0;
    active.x = 2;  // centrado-ish en board de 8 ancho con bitmap 4×4
    active.y = 0;
    if (collides(active)) {
      // No hay lugar para spawnear → game over.
      state = GAME_OVER;
    }
  }

  bool collides(const Piece &p) const {
    uint16_t bits = PIECES[p.type][p.rotation];
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        if (!(bits & (1 << (15 - r * 4 - c)))) continue;
        int bx = p.x + c;
        int by = p.y + r;
        if (bx < 0 || bx >= W) return true;
        if (by >= H) return true;
        // by < 0 no es colisión (piezas pueden spawnear con parte fuera arriba).
        if (by >= 0 && board[by][bx] != 0) return true;
      }
    }
    return false;
  }

  bool try_move(int dx, int dy) {
    Piece p = active;
    p.x += dx;
    p.y += dy;
    if (collides(p)) return false;
    active = p;
    return true;
  }

  void lock_piece() {
    uint16_t bits = PIECES[active.type][active.rotation];
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 4; c++) {
        if (!(bits & (1 << (15 - r * 4 - c)))) continue;
        int bx = active.x + c;
        int by = active.y + r;
        if (bx >= 0 && bx < W && by >= 0 && by < H) {
          board[by][bx] = active.type + 1;  // 0 reservado para vacío
        }
      }
    }
  }

  // Devuelve cuántas líneas se limpiaron. Compacta el board hacia abajo.
  int clear_lines() {
    int write_y = H - 1;
    int cleared = 0;
    for (int read_y = H - 1; read_y >= 0; read_y--) {
      bool full = true;
      for (int x = 0; x < W; x++) {
        if (board[read_y][x] == 0) { full = false; break; }
      }
      if (full) {
        cleared++;
        continue;  // saltear esa fila
      }
      if (write_y != read_y) {
        for (int x = 0; x < W; x++) board[write_y][x] = board[read_y][x];
      }
      write_y--;
    }
    // Limpiar las filas de arriba que quedaron desplazadas.
    for (int y = write_y; y >= 0; y--) {
      for (int x = 0; x < W; x++) board[y][x] = 0;
    }
    return cleared;
  }
};

inline TetrisGame game;  // C++17 inline → única instancia global

}  // namespace tetris
