#include "TitleScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "SrvManager.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG

#include <LightGroup.h>
#include <line/DrawLine3D.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    objCommon_ = Object3dCommon::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -10.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    wt1_.Initialize();
    wt2_.Initialize();

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("rostock_laage_airport_4k.dds");

    player_ = std::make_unique<Object3d>();
    player_->Initialize("walk.gltf");

    enemy_ = std::make_unique<Object3d>();
    enemy_->Initialize("sneakWalk.gltf");

    emitter_ = std::make_unique<ParticleEmitter>();
    emitter_->Initialize("test", "debug/ringPlane.obj");

    json_ = std::make_unique<JsonLoader>();
    json_->LoadSceneFile("test.json");

    // jsonからキャラクターの初期位置を設定
    const auto &players = json_->GetPlayers();
    if (!players.empty()) {
        const auto &playerData = players[0];
        wt1_.translation_ = playerData.translation;
        wt1_.rotation_ = playerData.rotation;
    }

    const auto &enemies = json_->GetEnemies();
    if (!enemies.empty()) {
        const auto &enemyData = enemies[0];
        wt2_.translation_ = enemyData.translation;
        wt2_.rotation_ = enemyData.rotation;
    }

    // 環境マッピング設定
    player_->GetModel()->SetEnvironmentSrvIndex(skybox_->GetTextureIndex());
    player_->SetRefrect(true);
}

void TitleScene::Finalize() {
}

void TitleScene::Update() {
#ifdef _DEBUG
    // デバッグ
    Debug();
#endif // _DEBUG

    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    emitter_->Update(vp_);
    player_->UpdateAnimation(roop);
    enemy_->UpdateAnimation(roop);

    skybox_->Update(vp_);

    wt1_.UpdateMatrix();
    wt2_.UpdateMatrix();

    json_->UpdateScene();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------

    skybox_->Draw();

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    objCommon_->skinningDrawCommonSetting();
    //-----アニメーションの描画開始-----
    player_->Draw(wt1_, vp_);
    enemy_->Draw(wt2_, vp_);
    //------------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----
    json_->DrawScene(vp_);
    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    //-----線描画-----
    DrawLine3D::GetInstance()->Draw(vp_);
    //---------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    objCommon_->skinningDrawCommonSetting();
    //-----アニメーションの描画開始-----

    //------------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----

    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::Debug() {
    ImGui::Begin("TitleScene:Debug");

    debugCamera_->imgui();

    LightGroup::GetInstance()->imgui();

    ImGui::Checkbox("roop", &roop);

    ImGui::End();

    emitter_->imgui();
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void TitleScene::ChangeScene() {
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("GAME");
    }
}
