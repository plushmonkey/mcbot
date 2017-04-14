#include "Inventory.h"

#include <mclib/protocol/packets/PacketDispatcher.h>

const s32 Inventory::HOTBAR_SLOT_START = 36;
const s32 Inventory::PLAYER_INVENTORY_ID = 0;

Inventory::Inventory(mc::core::Connection* connection, int windowId)
    : m_WindowId(windowId),
      m_Connection(connection),
      m_SelectedHotbarIndex(0)
{
    
}

mc::inventory::Slot* Inventory::GetSlot(s32 index) {
    auto iter = m_Inventory.find(index);
    if (iter == m_Inventory.end()) return nullptr;
    return &iter->second;
}

s32 Inventory::FindItemById(s32 itemId) {
    auto iter = std::find_if(m_Inventory.begin(), m_Inventory.end(), [&](const std::pair<s32, mc::inventory::Slot>& slot) {
        return slot.second.GetItemId() == itemId;
    });

    if (iter == m_Inventory.end()) return -1;
    return iter->first;
}

void Inventory::SelectHotbarSlot(s32 hotbarIndex) {
    if (m_WindowId != 0 || hotbarIndex < 0 || hotbarIndex > 8 || m_SelectedHotbarIndex == hotbarIndex) return;

    m_SelectedHotbarIndex = hotbarIndex;

    mc::protocol::packets::out::HeldItemChangePacket heldItemPacket(hotbarIndex);
    m_Connection->SendPacket(&heldItemPacket);
}

InventoryManager::InventoryManager(mc::protocol::packets::PacketDispatcher* dispatcher, mc::core::Connection* connection)
    : mc::protocol::packets::PacketHandler(dispatcher),
      m_Connection(connection)
{
    using namespace mc::protocol;
    dispatcher->RegisterHandler(State::Play, play::SetSlot, this);
    dispatcher->RegisterHandler(State::Play, play::HeldItemChange, this);
    dispatcher->RegisterHandler(State::Play, play::WindowItems, this);
}

InventoryManager::~InventoryManager() {
    GetDispatcher()->UnregisterHandler(this);
}

Inventory* InventoryManager::GetInventory(s32 windowId) {
    auto iter = m_Inventories.find(windowId);
    if (iter == m_Inventories.end()) return nullptr;
    return iter->second.get();
}

void InventoryManager::SetSlot(s32 windowId, s32 slotIndex, const mc::inventory::Slot& slot) {
    auto iter = m_Inventories.find(windowId);

    Inventory* inventory = nullptr;
    if (iter == m_Inventories.end()) {
        auto newInventory = std::make_unique<Inventory>(m_Connection, windowId);
        inventory = newInventory.get();
        m_Inventories[windowId] = std::move(newInventory);
    } else {
        inventory = iter->second.get();
    }

    inventory->m_Inventory[slotIndex] = slot;
}

void InventoryManager::HandlePacket(mc::protocol::packets::in::SetSlotPacket* packet) {
    SetSlot(packet->GetWindowId(), packet->GetSlotIndex(), packet->GetSlot());
}

void InventoryManager::HandlePacket(mc::protocol::packets::in::WindowItemsPacket* packet) {
    const std::vector<mc::inventory::Slot>& slots = packet->GetSlots();

    for (std::size_t i = 0; i < slots.size(); ++i) {
        SetSlot(packet->GetWindowId(), i, slots[i]);
    }
}

void InventoryManager::HandlePacket(mc::protocol::packets::in::HeldItemChangePacket* packet) {
    auto iter = m_Inventories.find(Inventory::PLAYER_INVENTORY_ID);

    if (iter == m_Inventories.end()) {
        auto inventory = std::make_unique<Inventory>(m_Connection, Inventory::PLAYER_INVENTORY_ID);

        iter = m_Inventories.insert(std::make_pair(Inventory::PLAYER_INVENTORY_ID, std::move(inventory))).first;
    }

    iter->second->SelectHotbarSlot(packet->GetSlot());
}
