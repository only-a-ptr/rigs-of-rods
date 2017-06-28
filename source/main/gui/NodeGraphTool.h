
#pragma once

#include "GUIManager.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // For ImRect

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
// External
#include <angelscript.h>

#include <list>
#include <string>
#include <vector>


namespace RoR {

struct Vec3 { float x, y, z; };

#define RoR_ARRAYSIZE(_ARR)  (sizeof(_ARR)/sizeof(*_ARR))

class NodeGraphTool
{
public:

    struct Style
    {
        ImU32 color_grid;
        float grid_line_width;
        float grid_size;
        ImU32 color_node;
        ImU32 color_node_frame;
        ImU32 color_node_active;
        ImU32 color_node_frame_active;
        ImU32 color_node_hovered;
        ImU32 color_node_frame_hovered;
        float node_rounding;
        ImVec2 node_window_padding;
        ImU32 color_input_slot;
        ImU32 color_input_slot_hover;
        ImU32 color_output_slot;
        ImU32 color_output_slot_hover;
        float node_slots_radius;
        ImU32 color_link;
        float link_line_width;
        ImVec2 slot_hoverbox_extent;

        Style();
    };

    struct Node; // Forward

    /// An output buffer of a node. 1 buffer = 1 output slot.
    struct Buffer
    {
        static const int SIZE = 2000; // Physics tick is 2Khz

        Buffer(int _slot): offset(0), slot(_slot)
        {
            memset(data, 0, sizeof(float)*Buffer::SIZE);
        }

        void            CopyKeepOffset(Buffer* src);                                ///< Copies source buffer as-is, including the offset; fastest
        void            CopyResetOffset(Buffer* src);                               ///< Copies source buffer, transforms the offset to 0
        void            Fill(const float* const src, int offset=0, int len=SIZE);
        inline void     Push(float entry)                                           { data[offset] = entry; this->Step(); }
        inline void     Step()                                                      { offset = (offset+1)%SIZE; }
        inline float    Read() const                                                { return data[offset]; }

        float data[Buffer::SIZE];
        int offset;
        int slot;
    };

    struct Link
    {
        Link(): node_src(nullptr), node_dst(nullptr), slot_dst(-1), buff_src(nullptr) {}

        Node* node_src;
        Node* node_dst;
        int slot_dst;
        Buffer* buff_src;
    };

    struct Node
    {
        enum class Type { INVALID, READING, GENERATOR, TRANSFORM, SCRIPT, DISPLAY, EULER, UDP };

        Node(NodeGraphTool* _graph, Type _type, ImVec2 _pos): graph(_graph), num_inputs(0), num_outputs(0), pos(_pos), type(_type), done(false)
        {
            id = _graph->AssignId();
        }

        inline ImVec2 GetInputSlotPos(size_t slot_idx)  { return ImVec2(pos.x,               pos.y + (calc_size.y * (static_cast<float>(slot_idx+1) / static_cast<float>(num_inputs+1)))); }
        inline ImVec2 GetOutputSlotPos(size_t slot_idx) { return ImVec2(pos.x + calc_size.x, pos.y + (calc_size.y * (static_cast<float>(slot_idx+1) / static_cast<float>(num_outputs+1)))); }

        virtual bool Process() { return true; }
        virtual void BindSrc(Link* link, int slot) {}
        virtual void BindDst(Link* link, int slot) {}
        virtual void DetachLink(Link* link) {}
        virtual void Draw() {}

        NodeGraphTool* graph;
        size_t num_inputs;
        size_t num_outputs;
        ImVec2 pos;
        ImVec2 draw_rect_min; // Updated by `DrawNodeBegin()`
        ImVec2 calc_size;
        ImVec2 user_size;
        int id;
        Type type;
        bool done; // Are data ready in this processing step?
    };

    /// reports XYZ position of node in world space
    /// Inputs: none
    /// Outputs(3): X position, Y position, Z position
    struct ReadingNode: public Node
    {
        ReadingNode(NodeGraphTool* _graph, ImVec2 _pos):
            Node(_graph, Type::READING, _pos), buffer_x(0), buffer_y(1), buffer_z(2), softbody_node_id(-1)
        {
            num_inputs = 0;
            num_outputs = 3;
        }

        inline void Push(Ogre::Vector3 pos)                 { buffer_x.Push(pos.x); buffer_y.Push(-pos.z); buffer_z.Push(pos.y); } // Transform OGRE coords -> classic coords
        virtual bool Process() override                             { this->done = true; return true; }
        virtual void BindSrc(Link* link, int slot) override;
        //           BindDst() not needed - no inputs
        virtual void DetachLink(Link* link);
        virtual void Draw() override;

        int softbody_node_id; // -1 means 'none'
        Buffer buffer_x;
        Buffer buffer_y;
        Buffer buffer_z;
    };

    struct GeneratorNode: public Node
    {
        GeneratorNode(NodeGraphTool* _graph, ImVec2 _pos):
            Node(_graph, Type::GENERATOR, _pos), amplitude(1.f), frequency(1.f), noise_max(0), elapsed(0.f), buffer_out(0)
        {
            num_inputs = 0;
            num_outputs = 1;
        }

        virtual bool Process() override                          { this->done = true; return true; }
        virtual void BindSrc(Link* link, int slot) override      { if (slot == 0) { link->buff_src = &buffer_out; link->node_src = this; } }
        //   BindDst() not needed - no inputs
        virtual void DetachLink(Link* link);
        virtual void Draw() override;

        float frequency; // Hz
        float amplitude;
        int noise_max;
        float elapsed;
        Buffer buffer_out;
    };

    struct ScriptNode: public Node
    {
        ScriptNode(NodeGraphTool* _nodegraph, ImVec2 _pos);
        
        virtual bool Process() override;                          ///< @return false if waiting for data, true if processed/nothing to process.
        virtual void BindSrc(Link* link, int slot) override;
        virtual void BindDst(Link* link, int slot) override;
        virtual void DetachLink(Link* link) override;
        virtual void Draw() override;

        // Script functions
        float Read(int slot, int offset);
        void  Write(int slot, float val);

        char code_buf[1000];
        AngelScript::asIScriptContext* script_context;
        AngelScript::asIScriptEngine*  script_engine;
        AngelScript::asIScriptFunction* script_func;
        char node_name[10];
        bool enabled; // Disables itself on script error
        Link* inputs[9];
        Buffer outputs[9];

    private:
        void InitScripting();
        void Apply();
    };

    struct EulerNode: public Node
    {
        EulerNode(NodeGraphTool* _nodegraph, ImVec2 _pos);
        
        virtual bool Process() override;                          ///< @return false if waiting for data, true if processed/nothing to process.
        virtual void BindSrc(Link* link, int slot) override;
        virtual void BindDst(Link* link, int slot) override;
        virtual void DetachLink(Link* link) override;
        virtual void Draw() override;

        Link* inputs[6]; // pitch axis XYZ, roll axis XYZ
        Buffer outputs[3]; // 
        bool error;
    };

    struct TransformNode: public Node
    {
        enum class Method
        {
            NONE, // Pass-through
            FIR_PLAIN,
            // TODO // FIR_ADAPTIVE_LMS,
            // TODO // FIR_ADAPTIVE_RLS,
            // TODO // FIR_ADAPTIVE_NLMS
        };

        TransformNode(NodeGraphTool* nodegraph, ImVec2 _pos);

        void ApplyFIR();

        bool Process() override;
        void BindSrc(Link* link, int slot) override      { if (slot == 0) { link->node_src = this; link->buff_src = &buffer_out; } }
        void BindDst(Link* link, int slot) override      { if (slot == 0) { link->node_dst = this; link->slot_dst = slot; link_in = link; } }
        void DetachLink(Link* link) override; // FINAL
        void Draw() override;

        char input_fir[100];
        char coefs_fir[100];
      //TODO  float adapt_rls_lambda;
      //TODO  float adapt_rls_p0;
      //TODO  float adapt_nlms_step;
      //TODO  float adapt_nlms_regz; // "regularization factor"
        Method method;
        Link* link_in;
        Buffer buffer_out;
        bool done;
    };

    struct DisplayNode: public Node
    {
        DisplayNode(NodeGraphTool* nodegraph, ImVec2 _pos);

        bool Process() override                             { this->done = true; return true; }
        //   BindSrc() - this node has no outputs.
        void BindDst(Link* link, int slot) override         { if (slot == 0) { link->node_dst = this; link->slot_dst = slot; link_in = link; } }
        void DetachLink(Link* link) override; // FINAL
        void Draw() override;

        Link* link_in;
        float plot_extent; // both min and max
    };

    /// Sink of the graph. Each field in UDP packet has one instance. Cannot be created by user, created automatically. Not placed in 'm_nodes' array.
    struct UdpNode: public Node
    {
        UdpNode(NodeGraphTool* nodegraph, ImVec2 _pos, const char* _title, const char* _desc);

        bool Process() override                             { this->done = true; return true; }
        //   BindSrc() - this node has no outputs.
        void BindDst(Link* link, int slot) override;
        void DetachLink(Link* link) override;
        void Draw() override;

        inline float Capture(int slot)                      { if (inputs[slot] != nullptr) { return inputs[slot]->buff_src->Read(); } else { return 0.f; } }

        Link* inputs[3];
        const char* title;
        const char* desc;
    };


    NodeGraphTool();

    void            Draw();
    void            PhysicsTick(Beam* actor);
    void            CalcGraph();
    void            SaveAsJson();                                                        ///< Filename specified by `m_filename`
    void            LoadFromJson();                                                      ///< Filename specified by `m_filename`
    void            SetFilename(const char* const filename)                              { strncpy(m_filename, filename, sizeof(m_filename)); }
    void            ClearAll();
    void            SetVisible(bool vis)                                                 { m_panel_visible = vis; }
    bool            IsVisible() const                                                    { return m_panel_visible; }

private:

    enum class HeaderMode { NORMAL, SAVE_FILE, LOAD_FILE, CLEAR_ALL };

    inline bool     IsInside (ImVec2 min, ImVec2 max, ImVec2 point) const                { return ((point.x > min.x) && (point.y > min.y)) && ((point.x < max.x) && (point.y < max.y)); }
    inline bool     IsLinkDragInProgress () const                                        { return (m_link_mouse_src != nullptr) || (m_link_mouse_dst != nullptr); }
    inline bool     IsRectHovered(ImVec2 min, ImVec2 max) const                          { return this->IsInside(min, max, m_nodegraph_mouse_pos); }
    inline void     DrawInputSlot (Node* node, const int index)                          { this->DrawSlotUni(node, index, true); }
    inline void     DrawOutputSlot (Node* node, const int index)                         { this->DrawSlotUni(node, index, false); }
    inline int      AssignId()                                                           { return m_free_id++; }
    inline void     UpdateFreeId(int existing_id)                                        { if (existing_id >= m_free_id) { m_free_id = (existing_id + 1); } }
    void            DrawSlotUni (Node* node, const int index, const bool input);
    Link*           AddLink (Node* src, Node* dst, int src_slot, int dst_slot);          ///< creates new link or fetches existing unused one
    Link*           FindLinkByDestination (Node* node, const int slot);
    Link*           FindLinkBySource (Node* node, const int slot);
    void            DrawNodeGraphPane ();
    void            DrawGrid ();
    void            DrawLink(Link* link);
    void            DeleteLink(Link* link);
    void            DeleteNode(Node* node);
    void            DrawNodeBegin(Node* node);
    void            DrawNodeFinalize(Node* node);
    void            AddMessage(const char* fmt, ...);
    void            NodeToJson(rapidjson::Value& j_data, Node* node, rapidjson::Document& doc);
    void            JsonToNode(Node* node, const rapidjson::Value& j_object);
    void            ScriptMessageCallback(const AngelScript::asSMessageInfo *msg, void *param);
    void            DetachAndDeleteNode(Node* node);


    inline bool IsSlotHovered(ImVec2 center_pos) const /// Slots can't use the "InvisibleButton" technique because it won't work when dragging.
    {
        ImVec2 min = center_pos - m_style.slot_hoverbox_extent;
        ImVec2 max = center_pos + m_style.slot_hoverbox_extent;
        return this->IsInside(min, max, m_nodegraph_mouse_pos);
    }

    std::vector<Node*>              m_nodes;
    std::vector<Link*>              m_links;
    std::list<std::string>          m_messages;
    char       m_directory[300];
    char       m_filename[100];
    char       m_motionsim_ip[40];
    int        m_motionsim_port;
    Style      m_style;
    ImVec2     m_scroll;
    ImVec2     m_scroll_offset;
    ImVec2     m_nodegraph_mouse_pos;
    Node*      m_hovered_node;
    Node*      m_context_menu_node;
    Node*      m_hovered_slot_node;
    int        m_hovered_slot_input;  // -1 = none
    int        m_hovered_slot_output; // -1 = none
    bool       m_is_any_slot_hovered;
    HeaderMode m_header_mode;
    Node*      m_last_scaled_node;
    TransformNode  m_fake_mouse_node;     ///< Used while dragging link with mouse. Type 'Transform' used just because we need anything with 1 input and 1 output.
    Link*      m_link_mouse_src;      ///< Link being mouse-dragged by it's input end.
    Link*      m_link_mouse_dst;      ///< Link being mouse-dragged by it's output end.
    int        m_free_id;
    bool       m_panel_visible;

public:
    UdpNode udp_position_node;
    UdpNode udp_velocity_node;
    UdpNode udp_accel_node;
    UdpNode udp_orient_node;
};

} // namespace RoR
