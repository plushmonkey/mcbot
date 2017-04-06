#include "AbilityAction.h"
#include "../../Pathfinder.h"
#include "../../Utility.h"
#include "../../components/PhysicsComponent.h"
#include "../../components/TargetingComponent.h"
#include "../../components/EffectComponent.h"

using mc::Vector3d;
using mc::Vector3i;

bool BackstabAction::CanAttack() {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    auto targeting = GetActorComponent(m_Client, TargetingComponent);

    auto entity = m_Client->GetEntityManager()->GetEntity(targeting->GetTargetEntity());
    if (!entity) return false;
    double orientation = physics->GetOrientation();
    float yaw = entity->GetYaw();

    Vector3d botRot = util::OrientationToVector(orientation);
    Vector3d tarRot = util::OrientationToVector(yaw);

    if (botRot.Dot(tarRot) > 0)
        return MeleeAction::CanAttack();
    return false;
}

bool SwiftKickAction::Attack(mc::entity::EntityPtr entity) {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    Vector3d pos = physics->GetPosition();
    Vector3d newPos = pos + Vector3d(0, 1.5, 0);
    physics->SetPosition(newPos);

    // Jump up one block before attacking so the bot is in the air for swift kick
    mc::protocol::packets::out::PlayerPositionAndLookPacket positionPacket(newPos,
        (float)physics->GetOrientation() * 180.0f / 3.14159f, 0.0f, false);
    m_Client->GetConnection()->SendPacket(&positionPacket);

    return MeleeAction::Attack(entity);
}

bool LungeAction::ShouldUse() {
    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    auto targeting = GetActorComponent(m_Client, TargetingComponent);

    auto entity = m_Client->GetEntityManager()->GetEntity(targeting->GetTargetEntity());
    if (!entity) return false;

    Vector3d target = entity->GetPosition();
    Vector3d position = physics->GetPosition();
    Vector3d direction = Vector3Normalize(target - position);

    double dist = position.Distance(target);
    if (dist < 12) return false;

    CastResult result = RayCast(m_Client->GetWorld(), m_Client->GetGraph(), position, direction, (std::size_t)dist);
    if (result.full)
        return MeleeAction::CanAttack();
    return false;
}

bool LungeAction::Attack(mc::entity::EntityPtr entity) {
    using namespace mc::protocol::packets::out;
    AnimationPacket animationPacket(mc::Hand::Main);
    m_Client->GetConnection()->SendPacket(&animationPacket);
    return true;
}

bool StanceAction::ShouldUse() {
    auto effects = GetActorComponent(m_Client, EffectComponent);

    const EffectComponent::EffectData* data = effects->GetEffectData(m_CheckEffect);
    bool hasEffect = data && data->amplifier >= m_EffectLevel;

    if (hasEffect) {
        m_ActivationTime = 0;
        return false;
    }

    s64 time = util::GetTime();
    
    if (m_ActivationTime == 0) {
        m_ActivationTime = time + 1000;
    }
    return true;
}

bool StanceAction::Attack(mc::entity::EntityPtr entity) {
    using namespace mc::protocol::packets::out;

    s64 time = util::GetTime();

    bool canAttack = MeleeAction::CanAttack() && time >= m_ActivationTime;

    
    if (canAttack) {
        m_ActivationTime = 0;

        AnimationPacket animationPacket(mc::Hand::Main);
        m_Client->GetConnection()->SendPacket(&animationPacket);
    }

    return canAttack;
}

Vector3d JoinArenaAction::GetSignNormal() {
    mc::block::BlockState bState = m_Client->GetWorld()->GetBlock(m_SignPosition);
    float angle = 0.0f;

    if (bState.GetBlock()->GetName().compare(L"Standing Sign Block") == 0) {
        u16 data = bState.GetData() & ((1 << 4) - 1);
        const float startAngle = (3.0f / 2.0f * 3.14159f);
        angle = startAngle - (2.0f * 3.14159f * data / 16.0f);
    } else if (bState.GetBlock()->GetName().compare(L"Wall-mounted Sign Block") == 0) {
        u16 data = bState.GetData() & ((1 << 4) - 1);
        switch (data) {
        case 2:
            angle = 0.5f * 3.1415f;
            break;
        case 3:
            angle = 3.0f / 2.0f * 3.1415f;
            break;
        case 4:
            angle = 3.1415f;
            break;
        case 5:
            angle = 0.0f;
            break;
        }
    }

    return Vector3d(std::cos(angle), 0, -std::sin(angle));
}

bool JoinArenaAction::FindSign() {
    if (m_FoundSign) return true;

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return false;

    if (m_MysticEmpire) {
        m_SignPosition = Vector3i(-16, 150, -49);
        m_FoundSign = true;

        const double StandAwayDistance = 1.5;

        Vector3d normal = GetSignNormal();
        Vector3i front = ToVector3i(ToVector3d(m_SignPosition) + normal * StandAwayDistance);
        m_StandPosition = GetGroundLevel(m_Client->GetWorld(), front);
        std::cout << "Found sign at " << m_SignPosition << " when bot at " << physics->GetPosition() << " StandPosition: " << m_StandPosition << std::endl;
        return true;
    } else {
        std::vector<mc::block::BlockEntityPtr> blockEntities = m_Client->GetWorld()->GetBlockEntities();

        for (mc::block::BlockEntityPtr blockEntity : blockEntities) {
            if (blockEntity->GetType() != mc::block::BlockEntityType::Sign) continue;

            std::shared_ptr<mc::block::SignBlockEntity> sign = std::static_pointer_cast<mc::block::SignBlockEntity>(blockEntity);

            for (std::size_t i = 0; i < 4; ++i) {
                std::wstring text = sign->GetText(i);
                std::transform(text.begin(), text.end(), text.begin(), towlower);

                std::wcout << text << std::endl;

                if (text.find(L"multi") != std::wstring::npos) {
                    m_SignPosition = sign->GetPosition();
                    m_FoundSign = true;

                    const double StandAwayDistance = 1.0;

                    Vector3d normal = GetSignNormal();
                    Vector3i front = ToVector3i(ToVector3d(m_SignPosition) + normal * StandAwayDistance);
                    m_StandPosition = GetGroundLevel(m_Client->GetWorld(), front);
                    std::cout << "Found sign at " << m_SignPosition << " when bot at " << physics->GetPosition() << std::endl;
                    return true;
                }
            }
        }

        return false;
    }
}

bool JoinArenaAction::IsNearSign() {
    if (!FindSign()) return false;

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return false;

    Vector3d position = physics->GetPosition();
    Vector3d toSign = ToVector3d(m_SignPosition) - position;
    const double NearbyRangeSq = 25 * 25;

    bool result = toSign.LengthSq() < NearbyRangeSq || (m_MysticEmpire && (s32)position.y == 149);

    std::cout << "Position: " << position << std::endl;
    return result;
}

void JoinArenaAction::Act() {
    auto targeting = GetActorComponent(m_Client, TargetingComponent);
    if (!targeting) return;

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return;

    targeting->SetTarget(m_StandPosition);
    targeting->SetTargetEntity(-1);

    ai::FaceSteering align(m_Client, ToVector3d(m_SignPosition), 0.1, 1, 1);
    physics->ApplyRotation(align.GetSteering().rotation);

    // Click on sign if nearby
    Vector3d toSign = ToVector3d(m_SignPosition) - physics->GetPosition();
    if (toSign.LengthSq() <= 4.0 * 4.0) {
        physics->SetVelocity(Vector3d(0, 0, 0));
        s64 time = util::GetTime();

        if (time >= m_LastClick + 2000) {
            using namespace mc::protocol::packets::out;
            std::cout << "Sending sign click" << std::endl;

            PlayerBlockPlacementPacket packet(m_SignPosition, 4, mc::Hand::Main, mc::Vector3f(0.5f, 0.0f, 0.5f));
            m_Client->GetConnection()->SendPacket(&packet);
            m_LastClick = time;
        }
    }
}

void FindTargetAction::Act()  {
    std::vector<mc::core::PlayerPtr> players;

    auto pm = m_Client->GetPlayerManager();
    for (auto iter = pm->begin(); iter != pm->end(); ++iter)
        players.push_back(iter->second);

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return;

    auto targeting = GetActorComponent(m_Client, TargetingComponent);
    if (!targeting) return;

    if (players.size() <= 1) {
        targeting->SetTargetEntity(-1);
        return;
    }

    double closestDist = std::numeric_limits<double>::max();
    mc::core::PlayerPtr closestPlayer;

    Vector3d position = physics->GetPosition();

    auto playerEntity = m_Client->GetEntityManager()->GetPlayerEntity();

    for (mc::core::PlayerPtr player : players) {
        auto entity = player->GetEntity();
        if (!entity || entity == playerEntity) continue;

        Vector3d toEntity = entity->GetPosition() - position;
        double distSq = toEntity.LengthSq();

        if (distSq < closestDist) {
            closestDist = distSq;
            closestPlayer = player;
        }
    }

    if (closestPlayer) {
        if (targeting->GetTargetEntity() != closestPlayer->GetEntity()->GetEntityId())
            std::wcout << L"Targeting " << closestPlayer->GetName() << std::endl;
        targeting->SetTargetEntity(closestPlayer->GetEntity()->GetEntityId());
        targeting->SetTarget(ToVector3i(closestPlayer->GetEntity()->GetPosition()));
    }
}

double FindTargetAction::DistanceToTarget() {
    Act();

    auto physics = GetActorComponent(m_Client, PhysicsComponent);
    if (!physics) return std::numeric_limits<double>::max();

    auto targeting = GetActorComponent(m_Client, TargetingComponent);
    if (!targeting) return std::numeric_limits<double>::max();;

    mc::core::PlayerPtr target = m_Client->GetPlayerManager()->GetPlayerByEntityId(targeting->GetTargetEntity());
    if (!target || !target->GetEntity()) return std::numeric_limits<double>::max();

    Vector3d toTarget = target->GetEntity()->GetPosition() - physics->GetPosition();

    return toTarget.Length();
}
