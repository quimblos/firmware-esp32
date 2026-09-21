#include "json.hpp"

const JSON JSON::empty = JSON({
    .kind=JSON::EMPTY,
    .value="",
    .children= {}
});