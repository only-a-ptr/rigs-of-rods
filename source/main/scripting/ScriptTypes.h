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

#pragma once

#ifdef USE_ANGELSCRIPT

#include "RoRPrerequisites.h"

#include <angelscript.h>
#include <Ogre.h>
#include <vector>

namespace RoR {

/// Proxy type for AngelScript to access arrays managed by the application
class ScriptArrayView
{
public:
    ScriptArrayView(void* data, size_t count, int size):
        m_data(data), m_num_elements(count), m_element_size(size)
    {}

    void* At(int index);

    size_t Length() { return m_num_elements; }

private:
    void*  m_data;
    int    m_element_size;
    size_t m_num_elements;
};

void RegisterScriptTypes(AngelScript::asIScriptEngine* engine);

} // namespace RoR

#endif // USE_ANGELSCRIPT
