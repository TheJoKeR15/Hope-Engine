#include "hpch.h"
#include "AssetImporter.h"

#include "ECS/Scene.h"
#include "ECS/Entity.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ECS\Components.h>

namespace HEngine
{
        void ProcessMesh(aiMesh* mesh, const aiScene* scene, Vector<FVertex>& vertices, Vector<uint32_t>& indices)
        {
            // walk through each of the mesh's vertices
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                FVertex vertex;
                Vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
                // positions
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;

                if (mesh->HasNormals())
                {
                Vec3 normal; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
// positions
                normal.x = mesh->mNormals[i].x;
                normal.y = mesh->mNormals[i].y;
                normal.z = mesh->mNormals[i].z;
                vertex.Normal = normal;
                }

                if (mesh->HasTextureCoords(0))
                {
                    Vec2 Coord; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
// positions
                    Coord.x = mesh->mTextureCoords[0][i].x;
                    Coord.y = mesh->mTextureCoords[0][i].y;
                    vertex.TexCoord = Coord;
                }

                vertices.push_back(vertex);
            }
            // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                // retrieve all indices of the face and store them in the indices vector
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }

        }
        void ProcessNode(aiNode* node,const aiScene* scene,Vector<FVertex>& vertices, Vector<uint32_t>& indices)
        {
            // process each mesh located at the current node
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                // the node object only contains indices to index the actual objects in the scene. 
                // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                ProcessMesh(mesh, scene, vertices, indices);
            }
            // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                ProcessNode(node->mChildren[i], scene, vertices, indices);
            }
        }

        void ProcessSceneMesh(aiMesh* mesh, const aiScene* scene, Scene* m_scene, float scale)
        {
            auto& entt = m_scene->CreateEntity(mesh->mName.C_Str());
            entt.GetComponent<TransformComponent>().Scale = Vec3(scale);
            Vector<FVertex> vertices{};
            Vector<uint32_t> indices{};
            ProcessMesh(mesh, scene, vertices, indices);
            auto& mrc = entt.AddComponent<MeshRendererComponent>(vertices, indices);
            mrc.material = m_scene->m_DefaultMaterial;
            mrc.bEmpty = false;

        }

        void ProcessSceneNode(aiNode* node, const aiScene* scene, Scene* m_scene, float scale)
        {
            // process each mesh located at the current node
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                // the node object only contains indices to index the actual objects in the scene. 
                // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                ProcessSceneMesh(mesh, scene,m_scene, scale);
            }
            // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                ProcessSceneNode(node->mChildren[i], scene,m_scene, scale);
            }
        }


        Model AssetImporter::ImportModel(std::string& path)
        {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                HERROR("ERROR WHEN IMPORTING ASSET (ASSIMP) = {0}", importer.GetErrorString());
                return Model{};
            }
            Model model = {};
            // process ASSIMP's root node recursively
            ProcessNode(scene->mRootNode, scene, model.verts, model.indices);
            return model;
        }

        void AssetImporter::ImportScene(std::string& path,Scene* m_scene, float scale)
        {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
            ProcessSceneNode(scene->mRootNode, scene, m_scene, scale);
        }

}