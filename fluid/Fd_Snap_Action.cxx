//
// Snap action code file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2023 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

#include "Fd_Snap_Action.h"

#include "Fl_Group_Type.h"
#include "alignment_panel.h"

#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Bar.H>
#include <math.h>
#include <string.h>

// TODO: selection should be initialised from Preferences to last selection
// TODO: how about a small tool box for quick preset selection and diabeling of individual snaps?
// TODO: pressed buttons are almost indistinguishable from normal buttons

void select_layout_suite_cb(Fl_Widget *, void *user_data);

int Fd_Snap_Action::eex = 0;
int Fd_Snap_Action::eey = 0;

Fd_Layout_Preset layout = {  // the currently active layout
  15, 15, 15, 15, 0, 0, // window:    l, r, t, b, gx, gy
  10, 10, 10, 10, 0, 0, // group:     l, r, t, b, gx, gy
  25, 25,               // tabs:      t, b
  20, 10, 4,            // widget_x:  min, inc, gap
  20,  4, 8,            // widget_y:  min, inc, gap
  0, 14, 0, 14          // labelfont/size, textfont/size
};

static Fd_Layout_Preset fltk_app = { 1 };
static Fd_Layout_Preset fltk_dlg = { 2 };
static Fd_Layout_Preset fltk_tool = { 3 };

static Fd_Layout_Preset grid_app = { 4 };
static Fd_Layout_Preset grid_dlg = { 5 };
static Fd_Layout_Preset grid_tool = { 6 };

static Fd_Layout_Suite static_suite_list[] = {
  { "FLTK", &fltk_app, &fltk_dlg, &fltk_tool, true },
  { "Grid", &grid_app, &grid_dlg, &grid_tool, true }
};

static Fl_Menu_Item static_main_menu[] = {
  { static_suite_list[0].name, 0, select_layout_suite_cb, (void*)0, FL_MENU_RADIO|FL_MENU_VALUE },
  { static_suite_list[1].name, 0, select_layout_suite_cb, (void*)1, FL_MENU_RADIO },
  { NULL }
};

static Fl_Menu_Item static_choice_menu[] = {
  { static_suite_list[0].name }, // TODO: save the last choosen suite in preferences
  { static_suite_list[1].name },
  { NULL }
};

Fd_Layout_List g_layout_list;

Fd_Layout_Preset &Fd_Layout_Preset::operator=(Fd_Layout_Preset const& rhs) {
  if (this != &rhs)
    ::memcpy(this, &rhs, sizeof(Fd_Layout_Preset));
  return *this;
}

void layout_suite_marker(Fl_Widget *, void *) {
  // never called
}

// TODO: this should be in alignment_panel.h
void edit_layout_suite_cb(Fl_Choice *w, void *user_data) {
  if (user_data == LOAD) {
    w->value(g_layout_list.current_suite());
  } else {
    int index = w->value();
    g_layout_list.current_suite(index);
    g_layout_list.update_dialogs();
  }
}

void select_layout_suite_cb(Fl_Widget *, void *user_data) {
  int index = (int)(fl_intptr_t)user_data;
  g_layout_list.current_suite(index);
  g_layout_list.update_dialogs();
}

void select_layout_preset_cb(Fl_Widget *, void *user_data) {
  int index = (int)(fl_intptr_t)user_data;
  g_layout_list.current_preset(index);
  g_layout_list.update_dialogs();
}

// TODO: this should be in alignment_panel.h
void edit_layout_preset_cb(Fl_Button *w, void *user_data) {
  int index = (int)(w - preset_choice[0]);
  if (user_data == LOAD) {
    w->value(g_layout_list.current_preset() == index);
  } else {
    g_layout_list.current_preset(index);
    g_layout_list.update_dialogs();
  }
}

Fd_Layout_List::Fd_Layout_List()
: main_menu_(static_main_menu),
  choice_menu_(static_choice_menu),
  list_(static_suite_list),
  list_size_(2),
  list_capacity_(2),
  list_is_static_(true),
  current_suite_(0),
  current_preset_(0)
{
}

Fd_Layout_List::~Fd_Layout_List() {
  if (!list_is_static_) {
    ::free(main_menu_);
    ::free(choice_menu_);
    ::free(list_);
  }
}

void Fd_Layout_List::update_dialogs() {
  static Fl_Menu_Item *preset_menu = NULL;
  if (!preset_menu)
    preset_menu = (Fl_Menu_Item*)main_menubar->find_item(select_layout_preset_cb);

  layout = *g_layout_list[g_layout_list.current_suite()].layout[g_layout_list.current_preset()];
  if (grid_window)
    grid_window->do_callback(grid_window, LOAD);
  preset_menu[g_layout_list.current_preset()].setonly();
  g_layout_list.main_menu_[g_layout_list.current_suite()].setonly();
}

/**
 Allocate enough space for n entries in the list.
 */
void Fd_Layout_List::capacity(int n) {
  static Fl_Menu_Item *suite_menu = NULL;
  if (!suite_menu)
    suite_menu = (Fl_Menu_Item*)main_menubar->find_item(layout_suite_marker);

  int old_n = list_size_;

  Fd_Layout_Suite *new_list = (Fd_Layout_Suite*)::calloc(n, sizeof(Fd_Layout_Suite));
  ::memcpy(new_list, list_, old_n * sizeof(Fd_Layout_Suite));
  if (!list_is_static_) ::free(list_);
  list_ = new_list;

  Fl_Menu_Item *new_main_menu = (Fl_Menu_Item*)::calloc(n+1, sizeof(Fl_Menu_Item));
  ::memcpy(new_main_menu, main_menu_, old_n * sizeof(Fl_Menu_Item));
  if (!list_is_static_) ::free(main_menu_);
  main_menu_ = new_main_menu;
  suite_menu->user_data(main_menu_);

  Fl_Menu_Item *new_choice_menu = (Fl_Menu_Item*)::calloc(n+1, sizeof(Fl_Menu_Item));
  ::memcpy(new_choice_menu, choice_menu_, old_n * sizeof(Fl_Menu_Item));
  if (!list_is_static_) ::free(choice_menu_);
  choice_menu_ = new_choice_menu;
  layout_choice->menu(choice_menu_);

  list_is_static_ = false;
}

/**
 Clone the currently selected suite and append it to the list.
 Selectes the new layout and updates the UI.
 */
int Fd_Layout_List::add(const char *name) {
  if (list_size_ == list_capacity_) {
    capacity(list_capacity_ * 2);
  }
  int n = list_size_;
  Fd_Layout_Suite &old_suite = list_[current_suite()];
  Fd_Layout_Suite new_suite;
  new_suite.name = strdup(name);
  for (int i=0; i<3; ++i) {
    new_suite.layout[i] = new Fd_Layout_Preset;
    new_suite.layout[i] = old_suite.layout[i];
  }
  new_suite.is_static = false;
  ::memcpy(list_+n, &new_suite, sizeof(Fd_Layout_Suite));
  main_menu_[n].label(new_suite.name);
  main_menu_[n].callback(main_menu_[0].callback());
  main_menu_[n].argument(n);
  main_menu_[n].flags = main_menu_[0].flags;
  choice_menu_[n].label(new_suite.name);
  list_size_ = n + 1;
  current_suite(n);
  update_dialogs();
  return 0;
}

//int Fd_Layout_Suite::add(Fd_Layout_Suite *suite) {
//  int new_size = list_size + 1;
//  Fd_Layout_Suite **new_list = (Fd_Layout_Suite**)::malloc(sizeof(Fd_Layout_Suite*) * new_size);
//  ::memcpy(new_list, list, sizeof(Fd_Layout_Suite*) * list_size);
//  new_list[list_size] = suite;
//  if (list_is_static)
//    list_is_static = false;
//  else
//    ::free(list);
//  list = new_list;
//  // TODO: update pulldown menus and the links to them (list_menu, unnamed)
//  // TODO: update menu pointers
//  // NOTE: maybe it is enough to allow for a fixed number of entries?
//}

// Presets: FLTK, Grid, ..., External
// Application
// Dialog
// Toolbox

static void draw_h_arrow(int, int, int);
static void draw_v_arrow(int x, int y1, int y2);
static void draw_left_brace(const Fl_Widget *w);
static void draw_right_brace(const Fl_Widget *w);
static void draw_top_brace(const Fl_Widget *w);
static void draw_bottom_brace(const Fl_Widget *w);
static void draw_grid(int x, int y, int dx, int dy);
static void draw_width(int x, int y, int r, Fl_Align a);
static void draw_height(int x, int y, int b, Fl_Align a);

static int fl_min(int a, int b) { return (a < b ? a : b); }
static int fl_min(int a, int b, int c) { return fl_min(a, fl_min(b, c)); }

static int nearest(int x, int left, int grid, int right=0x7fff) {
  int grid_x = ((x-left+grid/2)/grid)*grid+left;
  if (grid_x < left+grid/2) return left; // left+grid/2;
  if (grid_x > right-grid/2) return right; // right-grid/2;
  return grid_x;
}

static bool in_window(Fd_Snap_Data &d) {
  return (d.wgt && d.wgt->parent == d.win);
}

static bool in_group(Fd_Snap_Data &d) {
  return (d.wgt && d.wgt->parent && d.wgt->parent->is_group() && d.wgt->parent != d.win);
}

static bool in_tabs(Fd_Snap_Data &d) {
  return (d.wgt && d.wgt->parent && d.wgt->parent->is_tabs());
}

static Fl_Group *parent(Fd_Snap_Data &d) {
  return (d.wgt->o->parent());
}

/** \class Fd_Snap_Action
 */

int Fd_Snap_Action::check_x_(Fd_Snap_Data &d, int x_ref, int x_snap) {
  int dd = x_ref + d.dx - x_snap;
  int d2 = abs(dd);
  if (d2 > d.x_dist) return 1;
  dx = d.dx_out = d.dx - dd;
  ex = d.ex_out = x_snap;
  if (d2 == d.x_dist) return 0;
  d.x_dist = d2;
  return -1;
}

int Fd_Snap_Action::check_y_(Fd_Snap_Data &d, int y_ref, int y_snap) {
  int dd = y_ref + d.dy - y_snap;
  int d2 = abs(dd);
  if (d2 > d.y_dist) return 1;
  dy = d.dy_out = d.dy - dd;
  ey = d.ey_out = y_snap;
  if (d2 == d.y_dist) return 0;
  d.y_dist = d2;
  return -1;
}

void Fd_Snap_Action::check_x_y_(Fd_Snap_Data &d, int x_ref, int x_snap, int y_ref, int y_snap) {
  int ddx = x_ref + d.dx - x_snap;
  int d2x = abs(ddx);
  int ddy = y_ref + d.dy - y_snap;
  int d2y = abs(ddy);
  if ((d2x <= d.x_dist) && (d2y <= d.y_dist)) {
    dx = d.dx_out = d.dx - ddx;
    ex = d.ex_out = x_snap;
    d.x_dist = d2x;
    dy = d.dy_out = d.dy - ddy;
    ey = d.ey_out = y_snap;
    d.y_dist = d2y;
  }
}

bool Fd_Snap_Action::matches(Fd_Snap_Data &d) {
  switch (type) {
    case 1: return (d.drag & mask) && (eex == ex) && (d.dx == dx);
    case 2: return (d.drag & mask) && (eey == ey) && (d.dy == dy);
    case 3: return (d.drag & mask) && (eex == ex) && (d.dx == dx) && (eey == ey) && (d.dy == dy);
  }
  return false;
}

void Fd_Snap_Action::check_all(Fd_Snap_Data &data) {
  for (int i=0; list[i]; i++) {
    if (list[i]->mask & data.drag)
      list[i]->check(data);
  }
  eex = data.ex_out;
  eey = data.ey_out;
}

void Fd_Snap_Action::draw_all(Fd_Snap_Data &data) {
  for (int i=0; list[i]; i++) {
    if (list[i]->matches(data))
      list[i]->draw(data);
  }
}

/** Return a sensible step size for resizing a widget. */
void Fd_Snap_Action::get_resize_stepsize(int &x_step, int &y_step) {
  if ((layout.widget_inc_w > 1) && (layout.widget_inc_h > 1)) {
    x_step = layout.widget_inc_w;
    y_step = layout.widget_inc_h;
  } else if ((layout.group_grid_x > 1) && (layout.group_grid_y > 1)) {
    x_step = layout.group_grid_x;
    y_step = layout.group_grid_y;
  } else {
    x_step = layout.window_grid_x;
    y_step = layout.window_grid_y;
  }
}

/** Return a sensible step size for moving a widget. */
void Fd_Snap_Action::get_move_stepsize(int &x_step, int &y_step) {
  if ((layout.group_grid_x > 1) && (layout.group_grid_y > 1)) {
    x_step = layout.group_grid_x;
    y_step = layout.group_grid_y;
  } else if ((layout.window_grid_x > 1) && (layout.window_grid_y > 1)) {
    x_step = layout.window_grid_x;
    y_step = layout.window_grid_y;
  } else {
    x_step = layout.widget_gap_x;
    y_step = layout.widget_gap_y;
  }
}


// ---- snapping prototypes -------------------------------------------- MARK: -

class Fd_Snap_Left : public Fd_Snap_Action {
public:
  Fd_Snap_Left() { type = 1; mask = FD_LEFT|FD_DRAG; }
};

class Fd_Snap_Right : public Fd_Snap_Action {
public:
  Fd_Snap_Right() { type = 1; mask = FD_RIGHT|FD_DRAG; }
};

class Fd_Snap_Top : public Fd_Snap_Action {
public:
  Fd_Snap_Top() { type = 2; mask = FD_TOP|FD_DRAG; }
};

class Fd_Snap_Bottom : public Fd_Snap_Action {
public:
  Fd_Snap_Bottom() { type = 2; mask = FD_BOTTOM|FD_DRAG; }
};

// ---- window snapping ------------------------------------------------ MARK: -

class Fd_Snap_Left_Window_Edge : public Fd_Snap_Left {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE { clr(); check_x_(d, d.bx, 0); }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE { draw_left_brace(d.win->o); };
};
Fd_Snap_Left_Window_Edge snap_left_window_edge;

class Fd_Snap_Right_Window_Edge : public Fd_Snap_Right {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE { clr(); check_x_(d, d.br, d.win->o->w()); }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE { draw_right_brace(d.win->o); };
};
Fd_Snap_Right_Window_Edge snap_right_window_edge;

class Fd_Snap_Top_Window_Edge : public Fd_Snap_Top {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE { clr(); check_y_(d, d.by, 0); }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE { draw_top_brace(d.win->o); };
};
Fd_Snap_Top_Window_Edge snap_top_window_edge;

class Fd_Snap_Bottom_Window_Edge : public Fd_Snap_Bottom {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE { clr(); check_y_(d, d.bt, d.win->o->h()); }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE { draw_bottom_brace(d.win->o); };
};
Fd_Snap_Bottom_Window_Edge snap_bottom_window_edge;

class Fd_Snap_Left_Window_Margin : public Fd_Snap_Left {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_window(d)) check_x_(d, d.bx, layout.left_window_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_h_arrow(d.bx, (d.by+d.bt)/2, 0);
  };
};
Fd_Snap_Left_Window_Margin snap_left_window_margin;

class Fd_Snap_Right_Window_Margin : public Fd_Snap_Right {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_window(d)) check_x_(d, d.br, d.win->o->w()-layout.right_window_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_h_arrow(d.br, (d.by+d.bt)/2, d.win->o->w()-1);
  };
};
Fd_Snap_Right_Window_Margin snap_right_window_margin;

class Fd_Snap_Top_Window_Margin : public Fd_Snap_Top {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_window(d)) check_y_(d, d.by, layout.top_window_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_v_arrow((d.bx+d.br)/2, d.by, 0);
  };
};
Fd_Snap_Top_Window_Margin snap_top_window_margin;

class Fd_Snap_Bottom_Window_Margin : public Fd_Snap_Bottom {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_window(d)) check_y_(d, d.bt, d.win->o->h()-layout.bottom_window_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_v_arrow((d.bx+d.br)/2, d.bt, d.win->o->h()-1);
  };
};
Fd_Snap_Bottom_Window_Margin snap_bottom_window_margin;

// ---- group snapping ------------------------------------------------- MARK: -

class Fd_Snap_Left_Group_Edge : public Fd_Snap_Left {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_x_(d, d.bx, parent(d)->x());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_left_brace(parent(d));
  };
};
Fd_Snap_Left_Group_Edge snap_left_group_edge;

class Fd_Snap_Right_Group_Edge : public Fd_Snap_Right {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_x_(d, d.br, parent(d)->x() + parent(d)->w());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_right_brace(parent(d));
  };
};
Fd_Snap_Right_Group_Edge snap_right_group_edge;

class Fd_Snap_Top_Group_Edge : public Fd_Snap_Top {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_y_(d, d.by, parent(d)->y());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_top_brace(parent(d));
  };
};
Fd_Snap_Top_Group_Edge snap_top_group_edge;

class Fd_Snap_Bottom_Group_Edge : public Fd_Snap_Bottom {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_y_(d, d.bt, parent(d)->y() + parent(d)->h());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_bottom_brace(parent(d));
  };
};
Fd_Snap_Bottom_Group_Edge snap_bottom_group_edge;


class Fd_Snap_Left_Group_Margin : public Fd_Snap_Left {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_x_(d, d.bx, parent(d)->x() + layout.left_group_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_left_brace(parent(d));
    draw_h_arrow(d.bx, (d.by+d.bt)/2, parent(d)->x());
  };
};
Fd_Snap_Left_Group_Margin snap_left_group_margin;

class Fd_Snap_Right_Group_Margin : public Fd_Snap_Right {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d)) check_x_(d, d.br, parent(d)->x()+parent(d)->w()-layout.right_group_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_right_brace(parent(d));
    draw_h_arrow(d.br, (d.by+d.bt)/2, parent(d)->x()+parent(d)->w()-1);
  };
};
Fd_Snap_Right_Group_Margin snap_right_group_margin;

class Fd_Snap_Top_Group_Margin : public Fd_Snap_Top {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d) && !in_tabs(d)) check_y_(d, d.by, parent(d)->y()+layout.top_group_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_top_brace(parent(d));
    draw_v_arrow((d.bx+d.br)/2, d.by, parent(d)->y());
  };
};
Fd_Snap_Top_Group_Margin snap_top_group_margin;

class Fd_Snap_Bottom_Group_Margin : public Fd_Snap_Bottom {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_group(d) && !in_tabs(d)) check_y_(d, d.bt, parent(d)->y()+parent(d)->h()-layout.bottom_group_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_bottom_brace(parent(d));
    draw_v_arrow((d.bx+d.br)/2, d.bt, parent(d)->y()+parent(d)->h()-1);
  };
};
Fd_Snap_Bottom_Group_Margin snap_bottom_group_margin;

// ----- tabs snapping ------------------------------------------------- MARK: -

class Fd_Snap_Top_Tabs_Margin : public Fd_Snap_Top_Group_Margin {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_tabs(d)) check_y_(d, d.by, parent(d)->y()+layout.top_tabs_margin);
  }
};
Fd_Snap_Top_Tabs_Margin snap_top_tabs_margin;

class Fd_Snap_Bottom_Tabs_Margin : public Fd_Snap_Bottom_Group_Margin {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_tabs(d)) check_y_(d, d.bt, parent(d)->y()+parent(d)->h()-layout.bottom_tabs_margin);
  }
};
Fd_Snap_Bottom_Tabs_Margin snap_bottom_tabs_margin;

// ----- grid snapping ------------------------------------------------- MARK: -

class Fd_Snap_Grid : public Fd_Snap_Action {
protected:
  int nearest_x, nearest_y;
public:
  Fd_Snap_Grid() { type = 3; mask = FD_LEFT|FD_TOP|FD_DRAG; }
  void check_grid(Fd_Snap_Data &d, int left, int grid_x, int right, int top, int grid_y, int bottom) {
    if ((grid_x <= 1) || (grid_y <= 1)) return;
    int suggested_x = d.bx + d.dx;
    nearest_x = nearest(suggested_x, left, grid_x, right);
    int suggested_y = d.by + d.dy;
    nearest_y = nearest(suggested_y, top, grid_y, bottom);
    if (d.drag == FD_LEFT)
      check_x_(d, d.bx, nearest_x);
    else if (d.drag == FD_TOP)
      check_y_(d, d.by, nearest_y);
    else
      check_x_y_(d, d.bx, nearest_x, d.by, nearest_y);
  }
  bool matches(Fd_Snap_Data &d) FL_OVERRIDE {
    if (d.drag == FD_LEFT) return (eex == ex);
    if (d.drag == FD_TOP) return (eey == ey) && (d.dx == dx);
    return (d.drag & mask) && (eex == ex) && (d.dx == dx) && (eey == ey) && (d.dy == dy);
  }
};

class Fd_Snap_Window_Grid : public Fd_Snap_Grid {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (in_window(d)) check_grid(d, layout.left_window_margin, layout.window_grid_x, d.win->o->w()-layout.right_window_margin,
                                 layout.top_window_margin, layout.window_grid_y, d.win->o->h()-layout.bottom_window_margin);
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_grid(nearest_x, nearest_y, layout.window_grid_x, layout.window_grid_y);
  };
};
Fd_Snap_Window_Grid snap_window_grid;

class Fd_Snap_Group_Grid : public Fd_Snap_Grid {
public:
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    if (in_group(d)) {
      clr();
      Fl_Widget *g = parent(d);
      check_grid(d, g->x()+layout.left_group_margin, layout.group_grid_x, g->x()+g->w()-layout.right_group_margin,
                 g->y()+layout.top_group_margin, layout.group_grid_y, g->y()+g->h()-layout.bottom_group_margin);
    }
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_grid(nearest_x, nearest_y, layout.group_grid_x, layout.group_grid_y);
  };
};
Fd_Snap_Group_Grid snap_group_grid;

// ----- sibling snapping ---------------------------------------------- MARK: -

class Fd_Snap_Sibling : public Fd_Snap_Action {
protected:
  Fl_Widget *best_match;
public:
  Fd_Snap_Sibling() : best_match(NULL) { }
  virtual int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) = 0;
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    best_match = NULL;
    if (!d.wgt) return;
    if (!d.wgt->parent->is_group()) return;
    int dsib_min = 1024;
    Fl_Group_Type *gt = (Fl_Group_Type*)d.wgt->parent;
    Fl_Group *g = (Fl_Group*)gt->o;
    Fl_Widget *w = d.wgt->o;
    for (int i=0; i<g->children(); i++) {
      Fl_Widget *c = g->child(i);
      if (c == w) continue;
      int sret = sibling_check(d, c);
      if (sret < 1) {
        int dsib;
        if (type==1)
          dsib = abs( ((d.by+d.bt)/2+d.dy) - (c->y()+c->h()/2) );
        else
          dsib = abs( ((d.bx+d.br)/2+d.dx) - (c->x()+c->w()/2) );
        if (sret == -1 || (dsib < dsib_min)) {
          dsib_min = dsib;
          best_match = c;
        }
      }
    }
  }
};

class Fd_Snap_Siblings_Left_Same : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Left_Same() { type = 1; mask = FD_LEFT|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return check_x_(d, d.bx, s->x());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_left_brace(best_match);
  };
};
Fd_Snap_Siblings_Left_Same snap_siblings_left_same;

class Fd_Snap_Siblings_Left : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Left() { type = 1; mask = FD_LEFT|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return fl_min(check_x_(d, d.bx, s->x()+s->w()),
                  check_x_(d, d.bx, s->x()+s->w()+layout.widget_gap_x) );
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_right_brace(best_match);
  };
};
Fd_Snap_Siblings_Left snap_siblings_left;

class Fd_Snap_Siblings_Right_Same : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Right_Same() { type = 1; mask = FD_RIGHT|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return check_x_(d, d.br, s->x()+s->w());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_right_brace(best_match);
  };
};
Fd_Snap_Siblings_Right_Same snap_siblings_right_same;

class Fd_Snap_Siblings_Right : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Right() { type = 1; mask = FD_RIGHT|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return fl_min(check_x_(d, d.br, s->x()),
                  check_x_(d, d.br, s->x()-layout.widget_gap_x));
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_left_brace(best_match);
  };
};
Fd_Snap_Siblings_Right snap_siblings_right;

class Fd_Snap_Siblings_Top_Same : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Top_Same() { type = 2; mask = FD_TOP|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return check_y_(d, d.by, s->y());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_top_brace(best_match);
  };
};
Fd_Snap_Siblings_Top_Same snap_siblings_top_same;

class Fd_Snap_Siblings_Top : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Top() { type = 2; mask = FD_TOP|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return fl_min(check_y_(d, d.by, s->y()+s->h()),
                  check_y_(d, d.by, s->y()+s->h()+layout.widget_gap_y));
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_bottom_brace(best_match);
  };
};
Fd_Snap_Siblings_Top snap_siblings_top;

class Fd_Snap_Siblings_Bottom_Same : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Bottom_Same() { type = 2; mask = FD_BOTTOM|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return check_y_(d, d.bt, s->y()+s->h());
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_bottom_brace(best_match);
  };
};
Fd_Snap_Siblings_Bottom_Same snap_siblings_bottom_same;

class Fd_Snap_Siblings_Bottom : public Fd_Snap_Sibling {
public:
  Fd_Snap_Siblings_Bottom() { type = 2; mask = FD_BOTTOM|FD_DRAG; }
  int sibling_check(Fd_Snap_Data &d, Fl_Widget *s) FL_OVERRIDE {
    return fl_min(check_y_(d, d.bt, s->y()),
                  check_y_(d, d.bt, s->y()-layout.widget_gap_y));
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    if (best_match) draw_top_brace(best_match);
  };
};
Fd_Snap_Siblings_Bottom snap_siblings_bottom;


// ------ widget snapping ---------------------------------------------- MARK: -

class Fd_Snap_Widget_Ideal_Width : public Fd_Snap_Action {
public:
  Fd_Snap_Widget_Ideal_Width() { type = 1; mask = FD_LEFT|FD_RIGHT; }
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (!d.wgt) return;
    int iw = 15, ih = 15;
    d.wgt->ideal_size(iw, ih);
    if (d.drag == FD_RIGHT) {
      check_x_(d, d.br, d.bx+iw);
      iw = layout.widget_min_w;
      if (iw > 0) iw = nearest(d.br-d.bx+d.dx, layout.widget_min_w, layout.widget_inc_w);
      check_x_(d, d.br, d.bx+iw);
    } else {
      check_x_(d, d.bx, d.br-iw);
      iw = layout.widget_min_w;
      if (iw > 0) iw = nearest(d.br-d.bx-d.dx, layout.widget_min_w, layout.widget_inc_w);
      check_x_(d, d.bx, d.br-iw);
    }
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_width(d.bx, d.bt+7, d.br, 0);
  };
};
Fd_Snap_Widget_Ideal_Width snap_widget_ideal_width;

class Fd_Snap_Widget_Ideal_Height : public Fd_Snap_Action {
public:
  Fd_Snap_Widget_Ideal_Height() { type = 2; mask = FD_TOP|FD_BOTTOM; }
  void check(Fd_Snap_Data &d) FL_OVERRIDE {
    clr();
    if (!d.wgt) return;
    int iw, ih;
    d.wgt->ideal_size(iw, ih);
    if (d.drag == FD_BOTTOM) {
      check_y_(d, d.bt, d.by+ih);
      ih = layout.widget_min_h;
      if (ih > 0) ih = nearest(d.bt-d.by+d.dy, layout.widget_min_h, layout.widget_inc_h);
      check_y_(d, d.bt, d.by+ih);
    } else {
      check_y_(d, d.by, d.bt-ih);
      ih = layout.widget_min_h;
      if (ih > 0) ih = nearest(d.bt-d.by-d.dy, layout.widget_min_h, layout.widget_inc_h);
      check_y_(d, d.by, d.bt-ih);
    }
  }
  void draw(Fd_Snap_Data &d) FL_OVERRIDE {
    draw_height(d.br+7, d.by, d.bt, 0);
  };
};
Fd_Snap_Widget_Ideal_Height snap_widget_ideal_height;

// ---- snap actions list ---------------------------------------------- MARK: -

Fd_Snap_Action *Fd_Snap_Action::list[] = {
  &snap_left_window_edge,
  &snap_right_window_edge,
  &snap_top_window_edge,
  &snap_bottom_window_edge,

  &snap_left_window_margin,
  &snap_right_window_margin,
  &snap_top_window_margin,
  &snap_bottom_window_margin,

  &snap_window_grid,
  &snap_group_grid,

  &snap_left_group_edge,
  &snap_right_group_edge,
  &snap_top_group_edge,
  &snap_bottom_group_edge,

  &snap_left_group_margin,
  &snap_right_group_margin,
  &snap_top_group_margin,
  &snap_bottom_group_margin,

  &snap_top_tabs_margin,
  &snap_bottom_tabs_margin,

  &snap_siblings_left_same, &snap_siblings_left,
  &snap_siblings_right_same, &snap_siblings_right,
  &snap_siblings_top_same, &snap_siblings_top,
  &snap_siblings_bottom_same, &snap_siblings_bottom,

  &snap_widget_ideal_width,
  &snap_widget_ideal_height,

  NULL
};

// ---- draw alignment marks ------------------------------------------- MARK: -

static void draw_v_arrow(int x, int y1, int y2) {
  int dy = (y1>y2) ? -1 : 1 ;
  fl_yxline(x, y1, y2);
  fl_xyline(x-4, y2, x+4);
  fl_line(x-2, y2-dy*5, x, y2-dy);
  fl_line(x+2, y2-dy*5, x, y2-dy);
}

static void draw_h_arrow(int x1, int y, int x2) {
  int dx = (x1>x2) ? -1 : 1 ;
  fl_xyline(x1, y, x2);
  fl_yxline(x2, y-4, y+4);
  fl_line(x2-dx*5, y-2, x2-dx, y);
  fl_line(x2-dx*5, y+2, x2-dx, y);
}

static void draw_top_brace(const Fl_Widget *w) {
  int x = w->as_window() ? 0 : w->x();
  int y = w->as_window() ? 0 : w->y();
  fl_yxline(x, y-2, y+6);
  fl_yxline(x+w->w()-1, y-2, y+6);
  fl_xyline(x-2, y, x+w->w()+1);
}

static void draw_left_brace(const Fl_Widget *w)  {
  int x = w->as_window() ? 0 : w->x();
  int y = w->as_window() ? 0 : w->y();
  fl_xyline(x-2, y, x+6);
  fl_xyline(x-2, y+w->h()-1, x+6);
  fl_yxline(x, y-2, y+w->h()+1);
}

static void draw_right_brace(const Fl_Widget *w) {
  int x = w->as_window() ? w->w() - 1 : w->x() + w->w() - 1;
  int y = w->as_window() ? 0 : w->y();
  fl_xyline(x-6, y, x+2);
  fl_xyline(x-6, y+w->h()-1, x+2);
  fl_yxline(x, y-2, y+w->h()+1);
}

static void draw_bottom_brace(const Fl_Widget *w) {
  int x = w->as_window() ? 0 : w->x();
  int y = w->as_window() ? w->h() - 1 : w->y() + w->h() - 1;
  fl_yxline(x, y-6, y+2);
  fl_yxline(x+w->w()-1, y-6, y+2);
  fl_xyline(x-2, y, x+w->w()+1);
}

static void draw_height(int x, int y, int b, Fl_Align a) {
  char buf[16];
  int h = b - y;
  sprintf(buf, "%d", h);
  fl_font(FL_HELVETICA, 9);
  int lw = (int)fl_width(buf);
  int lx;

  b --;
  if (h < 30) {
    // Move height to the side...
    if (a == FL_ALIGN_LEFT) lx = x - lw - 2;
    else lx = x + 2;
    fl_yxline(x, y, b);
  } else {
    // Put height inside the arrows...
    if (a == FL_ALIGN_LEFT) lx = x - lw + 2;
    else lx = x - lw / 2;
    fl_yxline(x, y, y + (h - 11) / 2);
    fl_yxline(x, y + (h + 11) / 2, b);
  }

  // Draw the height...
  fl_draw(buf, lx, y + (h + 7) / 2);

  // Draw the arrowheads...
  fl_line(x-2, y+5, x, y+1, x+2, y+5);
  fl_line(x-2, b-5, x, b-1, x+2, b-5);

  // Draw the end lines...
  fl_xyline(x - 4, y, x + 4);
  fl_xyline(x - 4, b, x + 4);
}

static void draw_width(int x, int y, int r, Fl_Align a) {
  char buf[16];
  int w = r-x;
  sprintf(buf, "%d", w);
  fl_font(FL_HELVETICA, 9);
  int lw = (int)fl_width(buf);
  int ly = y + 4;

  r--;

  if (lw > (w - 20)) {
    // Move width above/below the arrows...
    if (a == FL_ALIGN_TOP) ly -= 10;
    else ly += 10;

    fl_xyline(x, y, r);
  } else {
    // Put width inside the arrows...
    fl_xyline(x, y, x + (w - lw - 2) / 2);
    fl_xyline(x + (w + lw + 2) / 2, y, r);
  }

  // Draw the width...
  fl_draw(buf, x + (w - lw) / 2, ly-2);

  // Draw the arrowheads...
  fl_line(x+5, y-2, x+1, y, x+5, y+2);
  fl_line(r-5, y-2, r-1, y, r-5, y+2);

  // Draw the end lines...
  fl_yxline(x, y - 4, y + 4);
  fl_yxline(r, y - 4, y + 4);
}

static void draw_grid(int x, int y, int dx, int dy) {
  int dx2 = 1, dy2 = 1;
  const int n = 2;
  for (int i=-n; i<=n; i++) {
    for (int j=-n; j<=n; j++) {
      if (abs(i)+abs(j) < 4) {
        int xx = x + i*dx , yy = y + j*dy;
        fl_xyline(xx-dx2, yy, xx+dx2);
        fl_yxline(xx, yy-dy2, yy+dy2);
      }
    }
  }
}
