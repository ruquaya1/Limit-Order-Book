#include "hplob/order_book.hpp"
#include <cassert>
#include <vector>

using namespace hplob;

static void test_basic_cross() {
  std::vector<Trade> trades;
  OrderBook ob([&](const Trade& t) { trades.push_back(t); });

  assert(ob.add(AddOrder{1, Side::Sell, 100, 10}));
  assert(ob.add(AddOrder{2, Side::Sell, 101, 10}));
  assert(ob.add(AddOrder{3, Side::Buy,  99, 10}));

  // Cross: buy at 101 should take 10@100 then 5@101
  assert(ob.add(AddOrder{10, Side::Buy, 101, 15}));

  assert(trades.size() == 2);
  assert(trades[0].price == 100 && trades[0].qty == 10);
  assert(trades[1].price == 101 && trades[1].qty == 5);

  auto top = ob.top();
  assert(top.has_ask);
  assert(top.ask_price == 101);
  assert(top.ask_qty == 5);
}

static void test_cancel_modify() {
  OrderBook ob;

  assert(ob.add(AddOrder{1, Side::Buy, 100, 10}));
  assert(ob.add(AddOrder{2, Side::Buy, 100, 20}));

  // modify same price keeps time priority, qty update ok
  assert(ob.modify(ModifyOrder{1, 100, 5}));

  auto top = ob.top();
  assert(top.has_bid);
  assert(top.bid_price == 100);
  assert(top.bid_qty == 25);

  assert(ob.cancel(CancelOrder{2}));
  top = ob.top();
  assert(top.bid_qty == 5);
}

int main() {
  test_basic_cross();
  test_cancel_modify();
  return 0;
}
