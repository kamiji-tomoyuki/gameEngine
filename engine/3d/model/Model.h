#pragma once
#include "ModelCommon.h"
#include "ModelStructs.h"
#include "SrvManager.h"
#include "animation/Animator.h"
#include "animation/Bone.h"
#include "animation/Skin.h"

#include "array"
#include "map"
#include "span"
#include "vector"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Matrix4x4.h"
#include "Quaternion.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include <unordered_set>

// モデル
class Model {
  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(ModelCommon *modelCommon, const std::string &directorypath, const std::string &filename);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

  public:
    /// <summary>
    /// .gltfか判別
    /// </summary>
    bool IsGltf() { return isGltf; }

    /// 各ステータス取得関数
    ModelDataEx GetModelData() { return modelDataEx_; }
    ModelData GetModelDataLegacy(); // 既存コードとの互換性用
    bool CheckBone() const { return hasBone_; }

    /// 各ステータス設定関数
    void SetSrv(SrvManager *srvManager) { srvManager_ = srvManager; }
    void SetAnimator(Animator *animator) { animator_ = animator; }
    void SetSkin(Skin *skin) { skin_ = skin; }
    void SetBone(Bone *bone) { bone_ = bone; }
    void SetEnvironmentSrvIndex(uint32_t index) { environmentSrvIndex = index; }
    void SetDefaultEnvironmentTexture(uint32_t srvIndex) { defaultEnvironmentSrvIndex = srvIndex; }

  private:
    /// <summary>
    /// メッシュ単位の頂点データ作成
    /// </summary>
    void CreateMeshResources(MeshData &mesh);

    /// <summary>
    /// .mtlファイルの読み取り
    /// </summary>
    static MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    ///  モデルファイルの読み取り
    /// </summary>
    static ModelDataEx LoadModelFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// ノード読み取り
    /// </summary>
    static Node ReadNode(aiNode *node);

  private:
    ModelCommon *modelCommon_;
    ModelDataEx modelDataEx_;
    SrvManager *srvManager_;

    std::string filename_;
    std::string directorypath_;

    static bool isGltf;
    Animator *animator_;
    Skin *skin_;
    Bone *bone_;

    bool hasBone_ = false;

    static std::unordered_set<std::string> jointNames;

    // 環境マッピング
    bool useEnvironmentMapping_;
    uint32_t defaultEnvironmentSrvIndex = 0;
    uint32_t environmentSrvIndex = 0;
};