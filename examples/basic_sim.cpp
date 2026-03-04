#include "hplob/order_book.hpp"
#include <iostream>

using namespace hplob;

int main() {
  std::size_t trades = 0;

  OrderBook ob([&](const Trade& tr) {
    ++trades;
    std::cout << "TRADE qty=" << tr.qty
              << " px=" << tr.price
              << " taker=" << tr.taker_id
              << " maker=" << tr.maker_id
              << " side=" << (tr.taker_side == Side::Buy ? "B" : "S")
              << "\n";
  });

  // Add resting liquidity
  ob.add(AddOrder{1, Side::Sell, 10100, 50});
  ob.add(AddOrder{2, Side::Sell, 10200, 30});
  ob.add(AddOrder{3, Side::Buy,   9900, 40});
  ob.add(AddOrder{4, Side::Buy,  10000, 25});

  auto t0 = ob.top();
  std::cout << "TOP bid=" << (t0.has_bid ? t0.bid_price : -1)
            << "@" << t0.bid_qty
            << " ask=" << (t0.has_ask ? t0.ask_price : -1)
            << "@" << t0.ask_qty << "\n";

  // Marketable buy (crosses asks)
  ob.add(AddOrder{10, Side::Buy, 10200, 60});

  auto t1 = ob.top();
  std::cout << "TOP bid=" << (t1.has_bid ? t1.bid_price : -1)
            << "@" << t1.bid_qty
            << " ask=" << (t1.has_ask ? t1.ask_price : -1)
            << "@" << t1.ask_qty << "\n";

  // Modify
  ob.modify(ModifyOrder{4, 10050, 25}); // price change loses priority
  // Cancel
  ob.cancel(CancelOrder{3});

  std::cout << "Live orders: " << ob.live_orders() << "\n";
  std::cout << "Trades: " << trades << "\n";
  return 0;
}
