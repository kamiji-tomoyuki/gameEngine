#pragma once
#include "DirectXCommon.h"
#include "PipeLineManager.h"
#include "SrvManager.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include <Matrix4x4.h>
#include <Vector2.h>
#include <Vector4.h>
#include <memory>
#include <string>

class Skybox {
  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(const std::string &textureFilePath);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update(const ViewProjection &viewProjection);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// ブレンドモード設定
    /// </summary>
    void SetBlendMode(BlendMode blendMode);

    /// <summary>
    /// スカイボックスのスケール設定
    /// </summary>
    void SetScale(float scale) { scale_ = scale; }

  private:
    /// <summary>
    /// 頂点データ作成
    /// </summary>
    void CreateVertexData();

    /// <summary>
    /// インデックスデータ作成
    /// </summary>
    void CreateIndexData();

    /// <summary>
    /// マテリアル作成
    /// </summary>
    void CreateMaterial();

    /// <summary>
    /// 変換行列作成
    /// </summary>
    void CreateTransformationMatrix();

  private:
    // --- 基盤 ---
    DirectXCommon *dxCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;
    std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

    // --- パイプライン ---
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];
    BlendMode blendMode_ = BlendMode::kNormal;

    // --- 頂点データ ---
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
    };

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    VertexData *vertexData = nullptr;

    // --- インデックスデータ ---
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    uint32_t *indexData = nullptr;

    // --- マテリアルデータ ---
    struct Material {
        Vector4 color;
        Matrix4x4 uvTransform;
        float padding[3];
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    Material *materialData = nullptr;

    // --- 変換行列データ ---
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;
    TransformationMatrix *transformationMatrixData = nullptr;

    // --- テクスチャ ---
    std::string textureFilePath_;
    uint32_t textureIndex_ = 0;

    // --- スカイボックスの設定 ---
    float scale_ = 1000.0f; // スカイボックスのスケール（非常に大きく設定）

    // --- UV座標（各面用） ---
    Vector2 uvs_[4] = {
        {0.0f, 0.0f}, // 左上
        {0.0f, 1.0f}, // 左下
        {1.0f, 1.0f}, // 右下
        {1.0f, 0.0f}  // 右上
    };

    // --- インデックスデータ ---
    std::vector<uint32_t> indices_;
};