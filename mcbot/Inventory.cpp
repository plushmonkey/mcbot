#include "Inventory.h"

#include <mclib/Packets/PacketDispatcher.h>

Inventory::Inventory(Minecraft::Connection* connection, int windowId)
    : m_WindowId(windowId),
      m_Connection(connection)
{
    
}

Minecraft::Slot* Inventory::GetSlot(s32 index) {
    auto iter = m_Inventory.find(index);
    if (iter == m_Inventory.end()) return nullptr;
    return &iter->second;
}

InventoryManager::InventoryManager(Minecraft::Packets::PacketDispatcher* dispatcher, Minecraft::Connection* connection)
    : Minecraft::Packets::PacketHandler(dispatcher),
      m_Connection(connection)
{
    using namespace Minecraft::Protocol;
    dispatcher->RegisterHandler(State::Play, Play::SetSlot, this);
    dispatcher->RegisterHandler(State::Play, Play::WindowItems, this);
}

InventoryManager::~InventoryManager() {
    GetDispatcher()->UnregisterHandler(this);
}

std::shared_ptr<Inventory> InventoryManager::GetInventory(s32 windowId) {
    auto iter = m_Inventories.find(windowId);
    if (iter == m_Inventories.end()) return nullptr;
    return iter->second;
}

void InventoryManager::SetSlot(s32 windowId, s32 slotIndex, const Minecraft::Slot& slot) {
    auto iter = m_Inventories.find(windowId);

    std::shared_ptr<Inventory> inventory;
    if (iter == m_Inventories.end()) {
        inventory = std::make_shared<Inventory>(m_Connection, windowId);
        m_Inventories[windowId] = inventory;
    } else {
        inventory = iter->second;
    }

    inventory->m_Inventory[slotIndex] = slot;
}

void InventoryManager::HandlePacket(Minecraft::Packets::Inbound::SetSlotPacket* packet) {
    SetSlot(packet->GetWindowId(), packet->GetSlotIndex(), packet->GetSlot());
}

void InventoryManager::HandlePacket(Minecraft::Packets::Inbound::WindowItemsPacket* packet) {
    const std::vector<Minecraft::Slot>& slots = packet->GetSlots();

    for (std::size_t i = 0; i < slots.size(); ++i) {
        SetSlot(packet->GetWindowId(), i, slots[i]);
    }
}
