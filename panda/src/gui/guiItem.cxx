// Filename: guiItem.cxx
// Created by:  cary (01Nov00)
// 
////////////////////////////////////////////////////////////////////

#include "guiItem.h"

TypeHandle GuiItem::_type_handle;

void GuiItem::recompute_frame(void) {
}

GuiItem::GuiItem(const string& name) : Namable(name), _added_hooks(false),
				       _scale(1.), _left(-1.), _right(1.),
				       _bottom(-1.), _top(1.),
				       _pos(0., 0., 0.),
				       _mgr((GuiManager*)0L) {
}

GuiItem::~GuiItem(void) {
  if (gui_cat->is_debug())
    gui_cat->debug() << "deleting item '" << this->get_name() << "'" << endl;
  this->unmanage();
}

void GuiItem::manage(GuiManager* mgr, EventHandler&) {
  _mgr = mgr;
}

void GuiItem::unmanage(void) {
  _mgr = (GuiManager*)0L;
}

void GuiItem::set_scale(float f) {
  _scale = f;
}

void GuiItem::set_pos(const LVector3f& p) {
  _pos = p;
}

void GuiItem::output(ostream& os) const {
  os << "GuiItem (0x" << (void*)this << ")" << endl;
  os << "  name - '" << get_name() << "'" << endl;
  os << "  hooks have" << (_added_hooks?" ":" not ") << "been added" << endl;
  os << "  scale - " << _scale << endl;
  os << "  pos - " << _pos << endl;
  os << "  mgr - 0x" << (void*)_mgr << endl;
  os << "  frame - (" << _left << ", " << _right << ", " << _bottom << ", "
     << _top << ")" << endl;
}
