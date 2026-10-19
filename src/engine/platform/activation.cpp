#include "platform/activation.hpp"

#include <string_view>

namespace lod
{
bool InputMods::Has(InputMod mod) const
{
  return (flags & static_cast<std::uint32_t>(mod)) != 0;
}

bool InputMods::ContainsAll(const InputMods& other) const
{
  return (flags & other.flags) == other.flags;
}

bool InputMods::Empty() const
{
  return flags == 0;
}

namespace
{
/// One row of a name table (reference constant name → enum value).
struct NameRow
{
  std::string_view name;
  std::uint32_t value;
};

template <typename EnumT>
struct NameTable
{
  const NameRow* rows;
  std::size_t size;

  [[nodiscard]] std::string_view NameOf(EnumT value) const
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      if (static_cast<std::uint32_t>(value) == rows[i].value)
      {
        return rows[i].name;
      }
    }
    return {};
  }

  [[nodiscard]] std::optional<EnumT> FromName(std::string_view name) const
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      if (name == rows[i].name)
      {
        return static_cast<EnumT>(rows[i].value);
      }
    }
    return std::nullopt;
  }
};

[[nodiscard]] const NameTable<InputKey>& InputKeyTable()
{
  static const std::vector<NameRow> rows = {
    {"RETURN", 0},
    {"ESCAPE", 1},
    {"BACKSPACE", 2},
    {"TAB", 3},
    {"SPACE", 4},
    {"EXCLAMATION", 5},
    {"DOUBLE_QUOTE", 6},
    {"HASH", 7},
    {"DOLLAR", 8},
    {"PERCENT", 9},
    {"AMPERSAND", 10},
    {"APOSTROPHE", 11},
    {"LEFT_PARENTHESIS", 12},
    {"RIGHT_PARENTHESIS", 13},
    {"ASTERISK", 14},
    {"PLUS", 15},
    {"COMMA", 16},
    {"MINUS", 17},
    {"PERIOD", 18},
    {"SLASH", 19},
    {"NUM_0", 20},
    {"NUM_1", 21},
    {"NUM_2", 22},
    {"NUM_3", 23},
    {"NUM_4", 24},
    {"NUM_5", 25},
    {"NUM_6", 26},
    {"NUM_7", 27},
    {"NUM_8", 28},
    {"NUM_9", 29},
    {"COLON", 30},
    {"SEMICOLON", 31},
    {"LESS", 32},
    {"EQUALS", 33},
    {"GREATER", 34},
    {"QUESTION", 35},
    {"AT", 36},
    {"LEFT_BRACKET", 37},
    {"BACKSLASH", 38},
    {"RIGHT_BRACKET", 39},
    {"CARET", 40},
    {"UNDERSCORE", 41},
    {"GRAVE", 42},
    {"A", 43},
    {"B", 44},
    {"C", 45},
    {"D", 46},
    {"E", 47},
    {"F", 48},
    {"G", 49},
    {"H", 50},
    {"I", 51},
    {"J", 52},
    {"K", 53},
    {"L", 54},
    {"M", 55},
    {"N", 56},
    {"O", 57},
    {"P", 58},
    {"Q", 59},
    {"R", 60},
    {"S", 61},
    {"T", 62},
    {"U", 63},
    {"V", 64},
    {"W", 65},
    {"X", 66},
    {"Y", 67},
    {"Z", 68},
    {"LEFT_BRACE", 69},
    {"PIPE", 70},
    {"RIGHT_BRACE", 71},
    {"TILDE", 72},
    {"DELETE", 73},
    {"PLUS_MINUS", 74},
    {"F1", 75},
    {"F2", 76},
    {"F3", 77},
    {"F4", 78},
    {"F5", 79},
    {"F6", 80},
    {"F7", 81},
    {"F8", 82},
    {"F9", 83},
    {"F10", 84},
    {"F11", 85},
    {"F12", 86},
    {"PRINT_SCREEN", 87},
    {"SCROLL_LOCK", 88},
    {"PAUSE", 89},
    {"INSERT", 90},
    {"HOME", 91},
    {"PAGE_UP", 92},
    {"END", 93},
    {"PAGE_DOWN", 94},
    {"RIGHT", 95},
    {"LEFT", 96},
    {"DOWN", 97},
    {"UP", 98},
    {"KP_DIVIDE", 99},
    {"KP_MULTIPLY", 100},
    {"KP_MINUS", 101},
    {"KP_PLUS", 102},
    {"KP_ENTER", 103},
    {"KP_1", 104},
    {"KP_2", 105},
    {"KP_3", 106},
    {"KP_4", 107},
    {"KP_5", 108},
    {"KP_6", 109},
    {"KP_7", 110},
    {"KP_8", 111},
    {"KP_9", 112},
    {"KP_0", 113},
    {"KP_PERIOD", 114},
    {"KP_EQUALS", 115},
    {"F13", 116},
    {"F14", 117},
    {"F15", 118},
    {"F16", 119},
    {"F17", 120},
    {"F18", 121},
    {"F19", 122},
    {"F20", 123},
    {"F21", 124},
    {"F22", 125},
    {"F23", 126},
    {"F24", 127},
    {"LEFT_CTRL", 128},
    {"LEFT_SHIFT", 129},
    {"LEFT_ALT", 130},
    {"LEFT_GUI", 131},
    {"RIGHT_CTRL", 132},
    {"RIGHT_SHIFT", 133},
    {"RIGHT_ALT", 134},
    {"RIGHT_GUI", 135},
  };
  static const NameTable<InputKey> table{rows.data(), rows.size()};
  return table;
}

[[nodiscard]] const NameTable<InputButton>& InputButtonTable()
{
  static const std::vector<NameRow> rows = {
    {"DPAD_UP", 0},
    {"DPAD_DOWN", 1},
    {"DPAD_LEFT", 2},
    {"DPAD_RIGHT", 3},
    {"A", 4},
    {"B", 5},
    {"X", 6},
    {"Y", 7},
    {"START", 8},
    {"SELECT", 9},
    {"LEFT_BUMPER", 10},
    {"RIGHT_BUMPER", 11},
    {"LEFT_STICK", 12},
    {"RIGHT_STICK", 13},
    {"GUIDE", 14},
    {"TOUCHPAD", 15},
    {"LEFT_PADDLE1", 16},
    {"RIGHT_PADDLE1", 17},
    {"LEFT_PADDLE2", 18},
    {"RIGHT_PADDLE2", 19},
    {"MISC1", 20},
    {"MISC2", 21},
    {"MISC3", 22},
    {"MISC4", 23},
    {"MISC5", 24},
    {"MISC6", 25},
  };
  static const NameTable<InputButton> table{rows.data(), rows.size()};
  return table;
}

[[nodiscard]] const NameTable<InputAxis>& InputAxisTable()
{
  static const std::vector<NameRow> rows = {
    {"LEFT_X", 0}, {"LEFT_Y", 1}, {"RIGHT_X", 2}, {"RIGHT_Y", 3}, {"LEFT_TRIGGER", 4}, {"RIGHT_TRIGGER", 5},
  };
  static const NameTable<InputAxis> table{rows.data(), rows.size()};
  return table;
}

} // namespace

std::string_view NameOf(InputKey key)
{
  return InputKeyTable().NameOf(key);
}

std::optional<InputKey> InputKeyFromName(std::string_view name)
{
  return InputKeyTable().FromName(name);
}

std::string_view NameOf(InputMod mod)
{
  switch (mod)
  {
    case InputMod::Ctrl:
      return "CTRL";
    case InputMod::Alt:
      return "ALT";
    case InputMod::Shift:
      return "SHIFT";
  }
  return {};
}

std::optional<InputMod> InputModFromName(std::string_view name)
{
  if (name == "CTRL")
  {
    return InputMod::Ctrl;
  }
  if (name == "ALT")
  {
    return InputMod::Alt;
  }
  if (name == "SHIFT")
  {
    return InputMod::Shift;
  }
  return std::nullopt;
}

std::string_view NameOf(InputButton button)
{
  return InputButtonTable().NameOf(button);
}

std::optional<InputButton> InputButtonFromName(std::string_view name)
{
  return InputButtonTable().FromName(name);
}

std::string_view NameOf(InputAxis axis)
{
  return InputAxisTable().NameOf(axis);
}

std::optional<InputAxis> InputAxisFromName(std::string_view name)
{
  return InputAxisTable().FromName(name);
}

std::string_view NameOf(InputAxisDirection direction)
{
  switch (direction)
  {
    case InputAxisDirection::Positive:
      return "POSITIVE";
    case InputAxisDirection::Negative:
      return "NEGATIVE";
  }
  return {};
}

std::optional<InputAxisDirection> InputAxisDirectionFromName(std::string_view name)
{
  if (name == "POSITIVE")
  {
    return InputAxisDirection::Positive;
  }
  if (name == "NEGATIVE")
  {
    return InputAxisDirection::Negative;
  }
  return std::nullopt;
}

} // namespace lod