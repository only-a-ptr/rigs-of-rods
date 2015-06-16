
#include "RigEditor_RigElementsAggregateData.h"

#include "RigEditor_Flare.h"
#include "RigDef_File.h" // For struct Flare2

using namespace RoR;
using namespace RigEditor;

void RigAggregateFlaresData::AddFlare(Flare* flare)
{
    ++num_elements;

    RigDef::Flare2& def = flare->GetDefinition();
    if (num_elements == 1)
    {
        ref_node              = def.reference_node;
        x_node                = def.node_axis_x;
        y_node                = def.node_axis_y;
        x_offset              = def.offset.x;
        y_offset              = def.offset.y;
        z_offset              = def.offset.z;
        size                  = def.size;
        flare_material_name   = def.material_name;
        type                  = def.type;
        control_number        = def.control_number;
        blink_delay_ms        = def.blink_delay_milis;

        materialflarebinding_material_name = flare->GetMaterialFlareBinding();
    }
    else
    {
        if (ref_node             != def.reference_node)    { this->SetIsRefNodeUniform         (false); }
        if (x_node               != def.node_axis_x)       { this->SetIsXNodeUniform           (false); }
        if (y_node               != def.node_axis_y)       { this->SetIsYNodeUniform           (false); }
        if (x_offset             != def.offset.x)          { this->SetIsXOffsetUniform         (false); }
        if (y_offset             != def.offset.y)          { this->SetIsYOffsetUniform         (false); }
        if (z_offset             != def.offset.z)          { this->SetIsZOffsetUniform         (false); }
        if (size                 != def.size)              { this->SetIsSizeUniform            (false); }
        if (flare_material_name  != def.material_name)     { this->SetIsFlareMatUniform        (false); }
        if (type                 != def.type)              { this->SetIsMatFlareBindingUniform (false); }
        if (control_number       != def.control_number)    { this->SetIsTypeUniform            (false); }
        if (blink_delay_ms       != def.blink_delay_milis) { this->SetIsControlNumberUniform   (false); }

        if (materialflarebinding_material_name != flare->GetMaterialFlareBinding()) 
        { 
            this->SetIsBlinkDelayUniform(false); 
        }
    }
}
