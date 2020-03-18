/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScriptTypes.h"

using namespace AngelScript;

void RoR::RegisterScriptTypes(asIScriptEngine* engine)
{
    engine->RegisterObjectType("array_view<class T>", sizeof(ScriptArrayView),
        asOBJ_VALUE | asOBJ_TEMPLATE | asGetTypeTraits<ScriptArrayView>());

    // The index operator returns the template subtype
    engine->RegisterObjectMethod("array_view<T>", "T &opIndex(uint index)", asMETHOD(ScriptArrayView, At), asCALL_THISCALL);

    engine->RegisterObjectMethod("array_view<T>", "uint length()", asMETHOD(ScriptArrayView, Length), asCALL_THISCALL);
}

void* RoR::ScriptArrayView::At(int index)
{
    assert(index >= 0 && index < m_num_elements);
    return ((char*)m_data + (index * m_element_size));
}

