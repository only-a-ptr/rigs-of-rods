/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "FlexBody.h"

#include "Application.h"
#include "ApproxMath.h"
#include "BeamData.h"
#include "FlexFactory.h"
#include "GfxActor.h"
#include "RigDef_File.h"

#include <Ogre.h>

using namespace Ogre;

FlexBody::FlexBody(
    RigDef::Flexbody* def,
    RoR::FlexBodyCacheData* preloaded_from_cache,
    RoR::GfxActor* gfx_actor,
    Ogre::MeshPtr mesh,
    int ref,
    int nx,
    int ny,
    Ogre::Quaternion const & rot,
    std::vector<unsigned int> & node_indices
):
      m_camera_mode(-2)
    , m_center_offset(def->offset)
    , m_node_center(ref)
    , m_node_x(nx)
    , m_node_y(ny)
    , m_has_texture_blend(false) // Disabled until further notice
    , m_scene_node(nullptr)
    , m_scene_entity(nullptr)
    , m_shared_buf_num_verts(0)
    , m_has_texture(true)
    , m_blend_changed(false)
    , m_locators(nullptr)
    , m_src_normals(nullptr)
    , m_dst_normals(nullptr)
    , m_dst_pos(nullptr)
    , m_src_colors(nullptr)
    , m_gfx_actor(gfx_actor)
{
    Vector3 normal = Vector3::UNIT_Y;
    Vector3 position = Vector3::ZERO;
    Quaternion orientation = Quaternion::ZERO;

    RoR::GfxActor::NodeData* nodes = m_gfx_actor->GetSimNodeBuffer();

    if (ref >= 0)
    {
        Vector3 diffX = nodes[nx].AbsPosition-nodes[ref].AbsPosition;
        Vector3 diffY = nodes[ny].AbsPosition-nodes[ref].AbsPosition;

        normal = (diffY.crossProduct(diffX)).normalisedCopy();

        // position
        position = nodes[ref].AbsPosition + def->offset.x * diffX + def->offset.y * diffY;
        position = position + def->offset.z * normal;

        // orientation
        Vector3 refX = diffX.normalisedCopy();
        Vector3 refY = refX.crossProduct(normal);
        orientation  = Quaternion(refX, normal, refY) * rot;
    }
    else
    {
        // special case!
        normal = Vector3::UNIT_Y;
        position = nodes[0].AbsPosition + def->offset;
        orientation = rot;
    }

    // Specify a vertex format (one-size-fits-all, for simplicity)

    Ogre::VertexDeclaration* optimalVD = Ogre::HardwareBufferManager::getSingleton().createVertexDeclaration();
    size_t offset = 0;
    // Position relative to 'ref/x/y' nodes, for deformation by vertex shader
    const Ogre::VertexElement& vd_pos = optimalVD->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    // Normal relative to 'ref/x/y' nodes, for deformation by vertex shader
    const Ogre::VertexElement& vd_norm = optimalVD->addElement(1, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    // Texture coordinates (aka UV coordinates) #0 - packed 'ref/x/y' node indices, for deformation by vertex shader
    const Ogre::VertexElement& vd_nodes = optimalVD->addElement(2, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
    // Texture coordinates (aka UV coordinates) #1 - actual texture coordinate data
    const Ogre::VertexElement& vd_uv = optimalVD->addElement(3, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);

    BufferUsageList optimalBufferUsages;
    for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u)
    {
        optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    }

    //reorg

    if (mesh->sharedVertexData)
    {
        mesh->sharedVertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
        mesh->sharedVertexData->removeUnusedBuffers();
        mesh->sharedVertexData->closeGapsInBindings();
    }
    Mesh::SubMeshIterator smIt = mesh->getSubMeshIterator();
    while (smIt.hasMoreElements())
    {
        SubMesh* sm = smIt.getNext();
        if (!sm->useSharedVertices)
        {
            sm->vertexData->reorganiseBuffers(optimalVD->clone(), optimalBufferUsages);
            sm->vertexData->removeUnusedBuffers();
            sm->vertexData->closeGapsInBindings();
        }
    }

    //getting vertex counts

    int num_submeshes = (int)mesh->getNumSubMeshes();
    if (preloaded_from_cache == nullptr)
    {
        m_vertex_count=0;
        m_uses_shared_vertex_data=false;
        m_num_submesh_vbufs=0;
        if (mesh->sharedVertexData)
        {
            m_vertex_count+=mesh->sharedVertexData->vertexCount;
            m_uses_shared_vertex_data=true;
        }
        for (int i=0; i<num_submeshes; i++)
        {
            if (!mesh->getSubMesh(i)->useSharedVertices)
            {
                m_vertex_count+=mesh->getSubMesh(i)->vertexData->vertexCount;
                m_num_submesh_vbufs++;
            }
        }
    } else
    {
        m_vertex_count            = preloaded_from_cache->header.vertex_count;
        m_uses_shared_vertex_data = preloaded_from_cache->header.UsesSharedVertexData();
        m_num_submesh_vbufs       = preloaded_from_cache->header.num_submesh_vbufs;
    }
    
    if (preloaded_from_cache != nullptr)
    {
        m_dst_pos     = preloaded_from_cache->dst_pos;
        m_src_normals = preloaded_from_cache->src_normals;
        m_locators    = preloaded_from_cache->locators;
        m_dst_normals = (Vector3*)malloc(sizeof(Vector3)*m_vertex_count); // Use malloc() for compatibility

        if (m_has_texture_blend)
        {
            m_src_colors = preloaded_from_cache->src_colors;
        }

        if (mesh->sharedVertexData)
        {
            m_shared_buf_num_verts=(int)mesh->sharedVertexData->vertexCount;

            //vertices
            m_shared_vbuf_pos=mesh->sharedVertexData->vertexBufferBinding->getBuffer(vd_pos.getSource());
            //normals
            m_shared_vbuf_norm=mesh->sharedVertexData->vertexBufferBinding->getBuffer(vd_norm.getSource());

        }
        unsigned int curr_submesh_idx = 0;
        for (int i=0; i<num_submeshes; i++)
        {
            const Ogre::SubMesh* submesh = mesh->getSubMesh(i);
            if (submesh->useSharedVertices)
            {
                continue;
            }
            const Ogre::VertexData* vertex_data = submesh->vertexData;
            m_submesh_vbufs_vertex_counts[curr_submesh_idx] = (int)vertex_data->vertexCount;

            m_submesh_vbufs_pos [curr_submesh_idx] = vertex_data->vertexBufferBinding->getBuffer(vd_pos.getSource());
            m_submesh_vbufs_norm[curr_submesh_idx] = vertex_data->vertexBufferBinding->getBuffer(vd_norm.getSource());

            curr_submesh_idx++;
        }
    }
    else
    {
        Ogre::Vector3* vertices=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_dst_pos=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_src_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_dst_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        if (m_has_texture_blend)
        {
            m_src_colors=(ARGB*)malloc(sizeof(ARGB)*m_vertex_count);
            for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
        }
        Vector3* vpt=vertices;
        Vector3* npt=m_src_normals;
        if (mesh->sharedVertexData)
        {
            m_shared_buf_num_verts=(int)mesh->sharedVertexData->vertexCount;
            //vertices
            m_shared_vbuf_pos=mesh->sharedVertexData->vertexBufferBinding->getBuffer(vd_pos.getSource());
            m_shared_vbuf_pos->readData(0, mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)vpt);
            vpt+=mesh->sharedVertexData->vertexCount;
            //normals
            m_shared_vbuf_norm=mesh->sharedVertexData->vertexBufferBinding->getBuffer(vd_norm.getSource());
            m_shared_vbuf_norm->readData(0, mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)npt);
            npt+=mesh->sharedVertexData->vertexCount;

        }
        int cursubmesh=0;
        for (int i=0; i<num_submeshes; i++)
        {
            const Ogre::SubMesh* submesh = mesh->getSubMesh(i);
            if (submesh->useSharedVertices)
            {
                continue;
            }
            const Ogre::VertexData* vertex_data = submesh->vertexData;
            int vertex_count = (int)vertex_data->vertexCount;
            m_submesh_vbufs_vertex_counts[cursubmesh] = vertex_count;
            //vertices
            m_submesh_vbufs_pos[cursubmesh]=vertex_data->vertexBufferBinding->getBuffer(vd_pos.getSource());
            m_submesh_vbufs_pos[cursubmesh]->readData(0, vertex_count*sizeof(Vector3), (void*)vpt);
            vpt += vertex_count;
            //normals
            m_submesh_vbufs_norm[cursubmesh]=vertex_data->vertexBufferBinding->getBuffer(vd_norm.getSource());
            m_submesh_vbufs_norm[cursubmesh]->readData(0, vertex_count*sizeof(Vector3), (void*)npt);
            npt += vertex_count;

            cursubmesh++;
        }

        //transform
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            vertices[i]=(orientation*vertices[i])+position;
        }

        m_locators = new Locator_t[m_vertex_count];
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            //search nearest node as the local origin
            float closest_node_distance = std::numeric_limits<float>::max();
            int closest_node_index = -1;
            for (auto node_index : node_indices)
            {
                float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                if (node_distance < closest_node_distance)
                {
                    closest_node_distance = node_distance;
                    closest_node_index = node_index;
                }
            }
            if (closest_node_index == -1)
            {
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": REF node not found");
                closest_node_index = 0;
            }
            m_locators[i].ref=closest_node_index;            

            //search the second nearest node as the X vector
            closest_node_distance = std::numeric_limits<float>::max();
            closest_node_index = -1;
            for (auto node_index : node_indices)
            {
                if (node_index == m_locators[i].ref)
                {
                    continue;
                }
                float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                if (node_distance < closest_node_distance)
                {
                    closest_node_distance = node_distance;
                    closest_node_index = node_index;
                }
            }
            if (closest_node_index == -1)
            {
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": VX node not found");
                closest_node_index = 0;
            }
            m_locators[i].nx=closest_node_index;

            //search another close, orthogonal node as the Y vector
            closest_node_distance = std::numeric_limits<float>::max();
            closest_node_index = -1;
            Vector3 vx = (nodes[m_locators[i].nx].AbsPosition - nodes[m_locators[i].ref].AbsPosition).normalisedCopy();
            for (auto node_index : node_indices)
            {
                if (node_index == m_locators[i].ref || node_index == m_locators[i].nx)
                {
                    continue;
                }
                float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                if (node_distance < closest_node_distance)
                {
                    Vector3 vt = (nodes[node_index].AbsPosition - nodes[m_locators[i].ref].AbsPosition).normalisedCopy();
                    float cost = vx.dotProduct(vt);
                    if (std::abs(cost) > std::sqrt(2.0f) / 2.0f)
                    {
                        continue; //rejection, fails the orthogonality criterion (+-45 degree)
                    }
                    closest_node_distance = node_distance;
                    closest_node_index = node_index;
                }
            }
            if (closest_node_index == -1)
            {
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": VY node not found");
                closest_node_index = 0;
            }
            m_locators[i].ny=closest_node_index;

            Matrix3 mat;
            Vector3 diffX = nodes[m_locators[i].nx].AbsPosition-nodes[m_locators[i].ref].AbsPosition;
            Vector3 diffY = nodes[m_locators[i].ny].AbsPosition-nodes[m_locators[i].ref].AbsPosition;

            mat.SetColumn(0, diffX);
            mat.SetColumn(1, diffY);
            mat.SetColumn(2, (diffX.crossProduct(diffY)).normalisedCopy()); // Old version: mat.SetColumn(2, nodes[loc.nz].AbsPosition-nodes[loc.ref].AbsPosition);

            mat = mat.Inverse();

            //compute coordinates in the newly formed Euclidean basis
            m_locators[i].coords = mat * (vertices[i] - nodes[m_locators[i].ref].AbsPosition);

            // that's it!
        }
        free(vertices);

    } // if (preloaded_from_cache == nullptr)

    //adjusting bounds
    AxisAlignedBox aab=mesh->getBounds();
    Vector3 v=aab.getMinimum();
    float mi=v.x;
    if (v.y<mi) mi=v.y;
    if (v.z<mi) mi=v.z;
    mi=fabs(mi);
    v=aab.getMaximum();
    float ma=v.x;
    if (ma<v.y) ma=v.y;
    if (ma<v.z) ma=v.z;
    ma=fabs(ma);
    if (mi>ma) ma=mi;
    aab.setMinimum(Vector3(-ma,-ma,-ma));
    aab.setMaximum(Vector3(ma,ma,ma));
    mesh->_setBounds(aab, true);

    m_scene_node=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    // Entity is created & attached by FlexFactory afterwards
    m_scene_node->setPosition(position);

    if (preloaded_from_cache == nullptr)
    {
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            Matrix3 mat;
            Vector3 diffX = nodes[m_locators[i].nx].AbsPosition-nodes[m_locators[i].ref].AbsPosition;
            Vector3 diffY = nodes[m_locators[i].ny].AbsPosition-nodes[m_locators[i].ref].AbsPosition;

            mat.SetColumn(0, diffX);
            mat.SetColumn(1, diffY);
            mat.SetColumn(2, diffX.crossProduct(diffY).normalisedCopy()); // Old version: mat.SetColumn(2, nodes[loc.nz].AbsPosition-nodes[loc.ref].AbsPosition);

            mat = mat.Inverse();

            // compute coordinates in the Euclidean basis
            m_src_normals[i] = mat*(orientation * m_src_normals[i]);
        }
    }

    // Transform the mesh for deformation via vertex shader
    int vert_offset = 0;
    if (mesh->sharedVertexData)
    {
        this->TransformVerticesForShaderDeformation(
            vert_offset, (int)mesh->sharedVertexData->vertexCount,
            m_shared_vbuf_pos, m_shared_vbuf_norm,
            mesh->sharedVertexData->vertexBufferBinding->getBuffer(vd_nodes.getSource()));

        vert_offset += (int)mesh->sharedVertexData->vertexCount;
    }
    for (int i = 0; i<num_submeshes; ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        if (!submesh->useSharedVertices)
        {
            this->TransformVerticesForShaderDeformation(
                vert_offset, (int)submesh->vertexData->vertexCount,
                m_submesh_vbufs_pos[i], m_submesh_vbufs_norm[i],
                submesh->vertexData->vertexBufferBinding->getBuffer(vd_nodes.getSource()));

            vert_offset += (int)submesh->vertexData->vertexCount;
        }
    }
    assert(vert_offset == m_vertex_count);
}


void FlexBody::TransformVerticesForShaderDeformation(
    int vert_offset, int vert_count,
    Ogre::HardwareVertexBufferSharedPtr vertices,
    Ogre::HardwareVertexBufferSharedPtr normals,
    Ogre::HardwareVertexBufferSharedPtr nodes_as_uv)
{
    Ogre::Vector3* vert_pos = (Ogre::Vector3*) vertices->lock(Ogre::HardwareBuffer::LockOptions::HBL_DISCARD);
    Ogre::Vector3* vert_norm = (Ogre::Vector3*) normals->lock(Ogre::HardwareBuffer::LockOptions::HBL_DISCARD);
    Ogre::Vector2* vert_nodes = (Ogre::Vector2*) nodes_as_uv->lock(Ogre::HardwareBuffer::LockOptions::HBL_DISCARD);

    for (int i = 0; i < vert_count; ++i)
    {
        // Vertex positions + normals must be relative to 'ref/x/y' nodes
        vert_pos[i] = m_locators[i + vert_offset].coords;
        vert_norm[i] = m_src_normals[i + vert_offset];
        // Node positions must be packed into a texcoord (aka UV) channel
        static_assert(sizeof(float) == sizeof(uint32_t), "float is not 32-bit?!");
        uint32_t u = (uint16_t)m_locators[i + vert_offset].ref << 16 | (uint16_t)m_locators[i + vert_offset].nx;
        uint32_t v = (uint16_t)m_locators[i + vert_offset].ny << 16 | (uint16_t)m_locators[i + vert_offset].nz;
        // by-the-book type punning, see https://gist.github.com/shafik/848ae25ee209f698763cffee272a58f8#how-do-we-type-pun-correctly
        std::memcpy(&vert_nodes[i].x, &u, sizeof(float));
        std::memcpy(&vert_nodes[i].y, &v, sizeof(float));
    }

    vertices->unlock();
    normals->unlock();
    nodes_as_uv->unlock();
}

FlexBody::~FlexBody()
{
    // Stuff using <new>
    if (m_locators != nullptr) { delete[] m_locators; }
    // Stuff using malloc()
    if (m_src_normals != nullptr) { free(m_src_normals); }
    if (m_dst_normals != nullptr) { free(m_dst_normals); }
    if (m_dst_pos     != nullptr) { free(m_dst_pos    ); }
    if (m_src_colors  != nullptr) { free(m_src_colors ); }

    // OGRE resource - scene node
    m_scene_node->getParentSceneNode()->removeChild(m_scene_node);
    gEnv->sceneManager->destroySceneNode(m_scene_node);
    m_scene_node = nullptr;

    // OGRE resource - scene entity
    Ogre::MeshPtr mesh = m_scene_entity->getMesh();
    gEnv->sceneManager->destroyEntity(m_scene_entity);
    m_scene_entity = nullptr;

    // OGRE resource - mesh (unique copy - should be destroyed)
    Ogre::MeshManager::getSingleton().remove(mesh->getHandle());
}

void FlexBody::setVisible(bool visible)
{
    if (m_scene_node)
        m_scene_node->setVisible(visible);
}

void FlexBody::SetFlexbodyCastShadow(bool val)
{
    m_scene_entity->setCastShadows(val);
}

void FlexBody::printMeshInfo(Mesh* mesh)
{
    if (mesh->sharedVertexData)
    {
        LOG("FLEXBODY Mesh has Shared Vertices:");
        VertexData* vt=mesh->sharedVertexData;
        LOG("FLEXBODY element count:"+TOSTRING(vt->vertexDeclaration->getElementCount()));
        for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
        {
            const VertexElement* ve=vt->vertexDeclaration->getElement(j);
            LOG("FLEXBODY element "+TOSTRING(j)+" source "+TOSTRING(ve->getSource()));
            LOG("FLEXBODY element "+TOSTRING(j)+" offset "+TOSTRING(ve->getOffset()));
            LOG("FLEXBODY element "+TOSTRING(j)+" type "+TOSTRING(ve->getType()));
            LOG("FLEXBODY element "+TOSTRING(j)+" semantic "+TOSTRING(ve->getSemantic()));
            LOG("FLEXBODY element "+TOSTRING(j)+" size "+TOSTRING(ve->getSize()));
        }
    }
    LOG("FLEXBODY Mesh has "+TOSTRING(mesh->getNumSubMeshes())+" submesh(es)");
    for (int i=0; i<mesh->getNumSubMeshes(); i++)
    {
        SubMesh* submesh = mesh->getSubMesh(i);
        LOG("FLEXBODY SubMesh "+TOSTRING(i)+": uses shared?:"+TOSTRING(submesh->useSharedVertices));
        if (!submesh->useSharedVertices)
        {
            VertexData* vt=submesh->vertexData;
            LOG("FLEXBODY element count:"+TOSTRING(vt->vertexDeclaration->getElementCount()));
            for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
            {
                const VertexElement* ve=vt->vertexDeclaration->getElement(j);
                LOG("FLEXBODY element "+TOSTRING(j)+" source "+TOSTRING(ve->getSource()));
                LOG("FLEXBODY element "+TOSTRING(j)+" offset "+TOSTRING(ve->getOffset()));
                LOG("FLEXBODY element "+TOSTRING(j)+" type "+TOSTRING(ve->getType()));
                LOG("FLEXBODY element "+TOSTRING(j)+" semantic "+TOSTRING(ve->getSemantic()));
                LOG("FLEXBODY element "+TOSTRING(j)+" size "+TOSTRING(ve->getSize()));
            }
        }
    }
}

void FlexBody::ComputeFlexbody()
{
    if (m_has_texture_blend) updateBlend();

    RoR::GfxActor::NodeData* nodes = m_gfx_actor->GetSimNodeBuffer();

    // compute the local center
    Ogre::Vector3 flexit_normal;

    if (m_node_center >= 0)
    {
        Vector3 diffX = nodes[m_node_x].AbsPosition - nodes[m_node_center].AbsPosition;
        Vector3 diffY = nodes[m_node_y].AbsPosition - nodes[m_node_center].AbsPosition;
        flexit_normal = fast_normalise(diffY.crossProduct(diffX));

        m_flexit_center = nodes[m_node_center].AbsPosition + m_center_offset.x * diffX + m_center_offset.y * diffY;
        m_flexit_center += m_center_offset.z * flexit_normal;
    }
    else
    {
        flexit_normal = Vector3::UNIT_Y;
        m_flexit_center = nodes[0].AbsPosition;
    }

    for (int i=0; i<(int)m_vertex_count; i++)
    {
        Vector3 diffX = nodes[m_locators[i].nx].AbsPosition - nodes[m_locators[i].ref].AbsPosition;
        Vector3 diffY = nodes[m_locators[i].ny].AbsPosition - nodes[m_locators[i].ref].AbsPosition;
        Vector3 nCross = fast_normalise(diffX.crossProduct(diffY)); //nCross.normalise();

        m_dst_pos[i].x = diffX.x * m_locators[i].coords.x + diffY.x * m_locators[i].coords.y + nCross.x * m_locators[i].coords.z;
        m_dst_pos[i].y = diffX.y * m_locators[i].coords.x + diffY.y * m_locators[i].coords.y + nCross.y * m_locators[i].coords.z;
        m_dst_pos[i].z = diffX.z * m_locators[i].coords.x + diffY.z * m_locators[i].coords.y + nCross.z * m_locators[i].coords.z;

        m_dst_pos[i] += nodes[m_locators[i].ref].AbsPosition - m_flexit_center;

        m_dst_normals[i].x = diffX.x * m_src_normals[i].x + diffY.x * m_src_normals[i].y + nCross.x * m_src_normals[i].z;
        m_dst_normals[i].y = diffX.y * m_src_normals[i].x + diffY.y * m_src_normals[i].y + nCross.y * m_src_normals[i].z;
        m_dst_normals[i].z = diffX.z * m_src_normals[i].x + diffY.z * m_src_normals[i].y + nCross.z * m_src_normals[i].z;

        m_dst_normals[i] = fast_normalise(m_dst_normals[i]);
    }
}

void FlexBody::UpdateFlexbodyVertexBuffers()
{
    Vector3 *ppt = m_dst_pos;
    Vector3 *npt = m_dst_normals;
    if (m_uses_shared_vertex_data)
    {
        m_shared_vbuf_pos->writeData(0, m_shared_buf_num_verts*sizeof(Vector3), ppt, true);
        ppt += m_shared_buf_num_verts;
        m_shared_vbuf_norm->writeData(0, m_shared_buf_num_verts*sizeof(Vector3), npt, true);
        npt += m_shared_buf_num_verts;
    }
    for (int i=0; i<m_num_submesh_vbufs; i++)
    {
        m_submesh_vbufs_pos[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(Vector3), ppt, true);
        ppt += m_submesh_vbufs_vertex_counts[i];
        m_submesh_vbufs_norm[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(Vector3), npt, true);
        npt += m_submesh_vbufs_vertex_counts[i];
    }

    if (m_blend_changed)
    {
        writeBlend();
        m_blend_changed = false;
    }

    m_scene_node->setPosition(m_flexit_center);
}

void FlexBody::reset()
{
    if (m_has_texture_blend)
    {
        for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
        writeBlend();
    }
}

void FlexBody::writeBlend()
{
    if (!m_has_texture_blend) return;
    ARGB *cpt = m_src_colors;
    if (m_uses_shared_vertex_data)
    {
        m_shared_vbuf_color->writeData(0, m_shared_buf_num_verts*sizeof(ARGB), (void*)cpt, true);
        cpt+=m_shared_buf_num_verts;
    }
    for (int i=0; i<m_num_submesh_vbufs; i++)
    {
        m_submesh_vbufs_color[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(ARGB), (void*)cpt, true);
        cpt+=m_submesh_vbufs_vertex_counts[i];
    }
}

void FlexBody::updateBlend() //so easy!
{
    RoR::GfxActor::NodeData* nodes = m_gfx_actor->GetSimNodeBuffer();
    for (int i=0; i<(int)m_vertex_count; i++)
    {
        RoR::GfxActor::NodeData *nd = &nodes[m_locators[i].ref];
        ARGB col = m_src_colors[i];
        if (nd->nd_has_contact && !(col&0xFF000000))
        {
            m_src_colors[i]=col|0xFF000000;
            m_blend_changed = true;
        }
        if (nd->nd_is_wet ^ ((col&0x000000FF)>0))
        {
            m_src_colors[i]=(col&0xFFFFFF00)+0x000000FF*nd->nd_is_wet;
            m_blend_changed = true;
        }
    }
}

