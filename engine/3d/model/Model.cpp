#include "Model.h"
#include "fstream"
#include "sstream"

#include "Frame.h"
#include "TextureManager.h"

#include "myMath.h"

bool Model::isGltf = false;
std::unordered_set<std::string> Model::jointNames = {};

void Model::Initialize(ModelCommon *modelCommon, const std::string &directorypath, const std::string &filename) {
    modelCommon_ = modelCommon;
    directorypath_ = directorypath;
    filename_ = filename;
    srvManager_ = SrvManager::GetInstance();

    modelDataEx_ = LoadModelFile(directorypath_, filename_);

    // 各メッシュのGPUリソースを作成
    for (auto &mesh : modelDataEx_.meshes) {
        CreateMeshResources(mesh);
    }

    // スキニングアニメーションか判別
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(directorypath_ + filename_, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    hasBone_ = false;
    if (scene) {
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            if (scene->mMeshes[i]->HasBones()) {
                hasBone_ = true;
                break;
            }
        }
    }

    // テクスチャ読み込み
    for (auto &material : modelDataEx_.materials) {
        TextureManager::GetInstance()->LoadTexture(material.textureFilePath);
        material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(material.textureFilePath);
    }

    animator_ = nullptr;
    bone_ = nullptr;
    skin_ = nullptr;

    useEnvironmentMapping_ = false;
    environmentSrvIndex = 0;

    if (defaultEnvironmentSrvIndex != 0) {
        environmentSrvIndex = defaultEnvironmentSrvIndex;
        useEnvironmentMapping_ = true;
    }
}

void Model::Draw() {
    // 環境マッピングのテクスチャを設定
    uint32_t envIndex = environmentSrvIndex;
    if (envIndex == 0 && defaultEnvironmentSrvIndex != 0) {
        envIndex = defaultEnvironmentSrvIndex;
    }

    if (envIndex != 0) {
        srvManager_->SetGraphicsRootDescriptorTable(6, envIndex);
    } else {
        uint32_t whiteTextureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/images/white1x1.png");
        srvManager_->SetGraphicsRootDescriptorTable(6, whiteTextureIndex);
    }

    // 各メッシュを描画
    for (const auto &mesh : modelDataEx_.meshes) {
        // マテリアルのテクスチャを設定
        if (mesh.materialIndex < modelDataEx_.materials.size()) {
            srvManager_->SetGraphicsRootDescriptorTable(2, modelDataEx_.materials[mesh.materialIndex].textureIndex);
        }

        if (!animator_ || !animator_->HaveAnimation()) {
            // アニメーションなし
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&mesh.indexBufferView);
        } else if (!CheckBone() || !mesh.hasBones) {
            // アニメーションありだがボーンなし
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&mesh.indexBufferView);
        } else {
            // スキニングアニメーション
            D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
                mesh.vertexBufferView,
                skin_->GetSkinCluster().influenceBufferView};

            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&mesh.indexBufferView);
            srvManager_->SetGraphicsRootDescriptorTable(7, skin_->GetSrvIndex());
        }

        // 描画
        modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(mesh.indices.size()), 1, 0, 0, 0);
    }
}

void Model::CreateMeshResources(MeshData &mesh) {
    // 頂点バッファ作成
    mesh.vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * mesh.vertices.size());
    mesh.vertexBufferView.BufferLocation = mesh.vertexResource->GetGPUVirtualAddress();
    mesh.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * mesh.vertices.size());
    mesh.vertexBufferView.StrideInBytes = sizeof(VertexData);

    mesh.vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&mesh.vertexData));
    std::memcpy(mesh.vertexData, mesh.vertices.data(), sizeof(VertexData) * mesh.vertices.size());

    // インデックスバッファ作成
    mesh.indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * mesh.indices.size());
    mesh.indexBufferView.BufferLocation = mesh.indexResource->GetGPUVirtualAddress();
    mesh.indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * mesh.indices.size());
    mesh.indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    mesh.indexResource->Map(0, nullptr, reinterpret_cast<void **>(&mesh.indexData));
    std::memcpy(mesh.indexData, mesh.indices.data(), sizeof(uint32_t) * mesh.indices.size());
}

MaterialData Model::LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
    MaterialData materialData;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            materialData.textureFilePath = directoryPath + "../images/" + textureFilename;
        }
    }

    if (materialData.textureFilePath.empty()) {
        materialData.textureFilePath = "resources/images/white1x1.png";
    }

    return materialData;
}

ModelDataEx Model::LoadModelFile(const std::string &directoryPath, const std::string &filename) {
    ModelDataEx modelDataEx;

    // gltfか判定
    isGltf = false;
    if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".gltf") {
        isGltf = true;
    } else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".obj") {
        isGltf = false;
    } else {
        assert(false && "Unsupported file format");
    }

    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene *scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

    if (!scene || !scene->HasMeshes()) {
        MaterialData defaultMaterial;
        defaultMaterial.textureFilePath = "resources/images/white1x1.png";
        modelDataEx.materials.push_back(defaultMaterial);
        return modelDataEx;
    }

    // マテリアル処理
    modelDataEx.materials.resize(scene->mNumMaterials);
    for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
        aiMaterial *material = scene->mMaterials[materialIndex];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
            aiString textureFilePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
            modelDataEx.materials[materialIndex].textureFilePath = directoryPath + textureFilePath.C_Str();
        } else {
            modelDataEx.materials[materialIndex].textureFilePath = "resources/images/white1x1.png";
        }
    }

    // マテリアルがない場合はデフォルトを追加
    if (modelDataEx.materials.empty()) {
        MaterialData defaultMaterial;
        defaultMaterial.textureFilePath = "resources/images/white1x1.png";
        modelDataEx.materials.push_back(defaultMaterial);
    }

    // メッシュ処理
    modelDataEx.meshes.resize(scene->mNumMeshes);
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh *mesh = scene->mMeshes[meshIndex];
        MeshData &meshData = modelDataEx.meshes[meshIndex];

        // マテリアルインデックスを保存
        meshData.materialIndex = mesh->mMaterialIndex;
        if (meshData.materialIndex >= modelDataEx.materials.size()) {
            meshData.materialIndex = 0;
        }

        // 法線、Texcoordがない場合エラーを出力
        assert(mesh->HasNormals());
        assert(mesh->HasTextureCoords(0));

        // 頂点データ
        meshData.vertices.resize(mesh->mNumVertices);
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            aiVector3D &position = mesh->mVertices[vertexIndex];
            aiVector3D &normal = mesh->mNormals[vertexIndex];
            aiVector3D &texcoord = mesh->mTextureCoords[0][vertexIndex];
            meshData.vertices[vertexIndex].position = {-position.x, position.y, position.z, 1.0f};
            meshData.vertices[vertexIndex].normal = {-normal.x, normal.y, normal.z};
            meshData.vertices[vertexIndex].texcoord = {texcoord.x, texcoord.y};
        }

        // インデックスデータ
        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace &face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                meshData.indices.push_back(vertexIndex);
            }
        }

        // ボーンデータ
        meshData.hasBones = mesh->HasBones();
        if (meshData.hasBones) {
            for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
                aiBone *bone = mesh->mBones[boneIndex];
                std::string jointName = bone->mName.C_Str();

                // 重複チェック（グローバルで管理）
                if (jointNames.find(jointName) == jointNames.end()) {
                    jointNames.insert(jointName);

                    aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
                    aiVector3D scale, translate;
                    aiQuaternion rotate;
                    bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

                    Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
                        {scale.x, scale.y, scale.z},
                        {rotate.x, -rotate.y, -rotate.z, rotate.w},
                        {-translate.x, translate.y, translate.z});

                    JointWeightData &jointWeightData = modelDataEx.skinClusterData[jointName];
                    jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

                    for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                        jointWeightData.vertexWeights.push_back({bone->mWeights[weightIndex].mWeight,
                                                                 bone->mWeights[weightIndex].mVertexId});
                    }
                }
            }
        }
    }

    // クリア
    jointNames.clear();

    modelDataEx.rootNode = ReadNode(scene->mRootNode);
    return modelDataEx;
}

Node Model::ReadNode(aiNode *node) {
    Node result;

    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);
    result.transform.scale = {scale.x, scale.y, scale.z};
    result.transform.rotate = {rotate.x, -rotate.y, -rotate.z, rotate.w};
    result.transform.translate = {-translate.x, translate.y, translate.z};

    result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

    result.name = node->mName.C_Str();
    result.children.resize(node->mNumChildren);
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }
    return result;
}

// 既存のObject3dコードとの互換性のための関数
ModelData Model::GetModelDataLegacy() {
    ModelData legacyData;

    // 最初のメッシュのデータを返す（単一メッシュとして扱う）
    if (!modelDataEx_.meshes.empty()) {
        legacyData.vertices = modelDataEx_.meshes[0].vertices;
        legacyData.indices = modelDataEx_.meshes[0].indices;
    }

    // 最初のマテリアルを返す
    if (!modelDataEx_.materials.empty()) {
        legacyData.material = modelDataEx_.materials[0];
    }

    legacyData.skinClusterData = modelDataEx_.skinClusterData;
    legacyData.rootNode = modelDataEx_.rootNode;

    return legacyData;
}