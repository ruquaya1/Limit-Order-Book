#include "hplob/order_book.hpp"
#include <cassert>

namespace hplob {

static inline bool is_valid_price(Price p) { return p != kInvalidPrice; }
static inline bool is_valid_qty(Qty q) { return q > 0; }

PriceLevel& OrderBook::level_for(Side s, Price p) {
  if (s == Side::Buy) {
    auto it = bids_.find(p);
    if (it == bids_.end()) {
      auto [ins, ok] = bids_.emplace(p, PriceLevel{p});
      (void)ok;
      return ins->second;
    }
    return it->second;
  } else {
    auto it = asks_.find(p);
    if (it == asks_.end()) {
      auto [ins, ok] = asks_.emplace(p, PriceLevel{p});
      (void)ok;
      return ins->second;
    }
    return it->second;
  }
}

PriceLevel* OrderBook::find_level(Side s, Price p) {
  if (s == Side::Buy) {
    auto it = bids_.find(p);
    return (it == bids_.end()) ? nullptr : &it->second;
  } else {
    auto it = asks_.find(p);
    return (it == asks_.end()) ? nullptr : &it->second;
  }
}

void OrderBook::remove_level_if_empty(Side s, Price p) {
  if (s == Side::Buy) {
    auto it = bids_.find(p);
    if (it != bids_.end() && it->second.empty()) bids_.erase(it);
  } else {
    auto it = asks_.find(p);
    if (it != asks_.end() && it->second.empty()) asks_.erase(it);
  }
}

bool OrderBook::match_incoming(OrderNode* taker) {
  if (taker->qty == 0) return true;

  if (taker->side == Side::Buy) {
    // Buy matches against best asks with price <= taker price
    while (taker->qty > 0 && !asks_.empty()) {
      auto best_it = asks_.begin(); // lowest ask
      Price best_px = best_it->first;
      if (best_px > taker->price) break;

      PriceLevel& lvl = best_it->second;
      while (taker->qty > 0 && lvl.head) {
        OrderNode* maker = lvl.head;

        Qty exec = (taker->qty < maker->qty) ? taker->qty : maker->qty;

        taker->qty -= exec;
        maker->qty -= exec;
        lvl.total_qty -= exec;

        if (on_trade_) {
          on_trade_(Trade{
            .taker_id = taker->id,
            .maker_id = maker->id,
            .taker_side = taker->side,
            .price = best_px,
            .qty = exec
          });
        }

        if (maker->qty == 0) {
          lvl.erase(maker);
          orders_by_id_.erase(maker->id);
          pool_.destroy(maker);
        }
      }

      if (lvl.empty()) asks_.erase(best_it);
    }
  } else {
    // Sell matches against best bids with price >= taker price
    while (taker->qty > 0 && !bids_.empty()) {
      auto best_it = bids_.begin(); // highest bid
      Price best_px = best_it->first;
      if (best_px < taker->price) break;

      PriceLevel& lvl = best_it->second;
      while (taker->qty > 0 && lvl.head) {
        OrderNode* maker = lvl.head;

        Qty exec = (taker->qty < maker->qty) ? taker->qty : maker->qty;

        taker->qty -= exec;
        maker->qty -= exec;
        lvl.total_qty -= exec;

        if (on_trade_) {
          on_trade_(Trade{
            .taker_id = taker->id,
            .maker_id = maker->id,
            .taker_side = taker->side,
            .price = best_px,
            .qty = exec
          });
        }

        if (maker->qty == 0) {
          lvl.erase(maker);
          orders_by_id_.erase(maker->id);
          pool_.destroy(maker);
        }
      }

      if (lvl.empty()) bids_.erase(best_it);
    }
  }

  return taker->qty == 0;
}

bool OrderBook::add(const AddOrder& a) {
  if (!is_valid_price(a.price) || !is_valid_qty(a.qty)) return false;
  if (orders_by_id_.find(a.id) != orders_by_id_.end()) return false;

  OrderNode* n = pool_.create();
  n->id = a.id;
  n->side = a.side;
  n->price = a.price;
  n->qty = a.qty;

  bool filled = match_incoming(n);
  if (filled) {
    pool_.destroy(n);
    return true;
  }

  PriceLevel& lvl = level_for(n->side, n->price);
  lvl.push_back(n);

  orders_by_id_.emplace(a.id, OrderRef{n});
  return true;
}

bool OrderBook::cancel(const CancelOrder& c) {
  auto it = orders_by_id_.find(c.id);
  if (it == orders_by_id_.end()) return false;

  OrderNode* n = it->second.node;
  PriceLevel* lvl = find_level(n->side, n->price);
  assert(lvl && "level must exist for a resting order");

  lvl->erase(n);
  orders_by_id_.erase(it);
  pool_.destroy(n);

  remove_level_if_empty(n->side, n->price);
  return true;
}

bool OrderBook::modify(const ModifyOrder& m) {
  auto it = orders_by_id_.find(m.id);
  if (it == orders_by_id_.end()) return false;
  if (!is_valid_price(m.new_price) || !is_valid_qty(m.new_qty)) return false;

  OrderNode* n = it->second.node;

  if (m.new_price == n->price) {
    PriceLevel* lvl = find_level(n->side, n->price);
    assert(lvl);

    if (m.new_qty == n->qty) return true;
    if (m.new_qty > n->qty) lvl->total_qty += (m.new_qty - n->qty);
    else lvl->total_qty -= (n->qty - m.new_qty);

    n->qty = m.new_qty;
    return true;
  }

  Side s = n->side;
  Qty q = m.new_qty;

  cancel(CancelOrder{m.id});
  return add(AddOrder{.id = m.id, .side = s, .price = m.new_price, .qty = q});
}

TopOfBook OrderBook::top() const {
  TopOfBook t{};

  if (!bids_.empty()) {
    const auto& [px, lvl] = *bids_.begin();
    t.has_bid = true;
    t.bid_price = px;
    t.bid_qty = lvl.total_qty;
  }
  if (!asks_.empty()) {
    const auto& [px, lvl] = *asks_.begin();
    t.has_ask = true;
    t.ask_price = px;
    t.ask_qty = lvl.total_qty;
  }
  return t;
}

}
