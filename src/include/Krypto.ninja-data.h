#ifndef K_DATA_H_
#define K_DATA_H_
//! \file
//! \brief Internal data objects.

namespace K {
  enum class mQuotingMode: unsigned int {
    Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth
  };
  enum class mQuotingSafety: unsigned int {
    Off, PingPong, Boomerang, AK47
  };
  enum class mQuoteState: unsigned int {
    Disconnected,  Live,             DisabledQuotes,
    MissingData,   UnknownHeld,      WidthMustBeSmaller,
    TBPHeld,       MaxTradesSeconds, WaitingPing,
    DepletedFunds, Crossed,
    UpTrendHeld,   DownTrendHeld
  };
  enum class mFairValueModel: unsigned int {
    BBO, wBBO, rwBBO
  };
  enum class mAutoPositionMode: unsigned int {
    Manual, EWMA_LS, EWMA_LMS, EWMA_4
  };
  enum class mPDivMode: unsigned int {
    Manual, Linear, Sine, SQRT, Switch
  };
  enum class mAPR: unsigned int {
    Off, Size, SizeWidth
  };
  enum class mSOP: unsigned int {
    Off, Trades, Size, TradesSize
  };
  enum class mSTDEV: unsigned int {
    Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff
  };
  enum class mPingAt: unsigned int {
    BothSides,    BidSide,         AskSide,
    DepletedSide, DepletedBidSide, DepletedAskSide,
    StopPings
  };
  enum class mPongAt: unsigned int {
    ShortPingFair,       AveragePingFair,       LongPingFair,
    ShortPingAggressive, AveragePingAggressive, LongPingAggressive
  };

  enum class mHotkey: unsigned int {
    ESC = 27,
     Q  = 81,
     q  = 113
  };

  enum class mPortal: unsigned char {
    Hello = '=',
    Kiss  = '-'
  };
  enum class mMatter: unsigned char {
    FairValue            = 'a', Quote                = 'b', ActiveSubscription = 'c', Connectivity       = 'd',
    MarketData           = 'e', QuotingParameters    = 'f', SafetySettings     = 'g', Product            = 'h',
    OrderStatusReports   = 'i', ProductAdvertisement = 'j', ApplicationState   = 'k', EWMAStats          = 'l',
    STDEVStats           = 'm', Position             = 'n', Profit             = 'o', SubmitNewOrder     = 'p',
    CancelOrder          = 'q', MarketTrade          = 'r', Trades             = 's', ExternalValuation  = 't',
    QuoteStatus          = 'u', TargetBasePosition   = 'v', TradeSafetyValue   = 'w', CancelAllOrders    = 'x',
    CleanAllClosedTrades = 'y', CleanAllTrades       = 'z', CleanTrade         = 'A',
    WalletChart          = 'C', MarketChart          = 'D', Notepad            = 'E',
                                MarketDataLongTerm   = 'H'
  };

  struct mToScreen {
    function<void(const string&, const string&)> print
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen print?"); }
#endif
    ;
    function<void(const string&, const string&, const string&)> focus
#ifndef NDEBUG
    = [](const string &prefix, const string &reason, const string &highlight) { WARN("Y U NO catch screen focus?"); }
#endif
    ;
    function<void(const string&, const string&)> warn
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen warn?"); }
#endif
    ;
    function<void()> refresh
#ifndef NDEBUG
    = []() { WARN("Y U NO catch screen refresh?"); }
#endif
    ;
  };

  struct mAbout {
    virtual const mMatter about() const = 0;
  };
  struct mBlob: virtual public mAbout {
    virtual const json blob() const = 0;
  };

  struct mFromClient: virtual public mAbout {
    virtual void kiss(json *const j) {};
  };

  struct mToClient: public mBlob,
                    public mFromClient {
    function<void()> send
#ifndef NDEBUG
    = []() { WARN("Y U NO catch client send?"); }
#endif
    ;
    virtual const json hello() {
      return { blob() };
    };
    virtual const bool realtime() const {
      return true;
    };
  };
  template <typename mData> struct mJsonToClient: public mToClient {
    virtual const bool send() {
      if ((send_asap() or send_soon())
        and (send_same_blob() or diff_blob())
      ) {
        send_now();
        return true;
      }
      return false;
    };
    virtual const json blob() const {
      return *(mData*)this;
    };
    protected:
      mClock send_last_Tstamp = 0;
      string send_last_blob;
      virtual const bool send_same_blob() const {
        return true;
      };
      const bool diff_blob() {
        const string last_blob = send_last_blob;
        return (send_last_blob = blob().dump()) != last_blob;
      };
      virtual const bool send_asap() const {
        return true;
      };
      const bool send_soon(const int &delay = 0) {
        const mClock now = Tstamp;
        if (send_last_Tstamp + max(369, delay) > now)
          return false;
        send_last_Tstamp = now;
        return true;
      };
      void send_now() const {
        if (mToClient::send)
          mToClient::send();
      };
  };

  struct mFromDb: public mBlob {
    function<void()> push
#ifndef NDEBUG
    = []() { WARN("Y U NO catch sqlite push?"); }
#endif
    ;
    virtual const bool pull(const json &j) = 0;
    virtual const string increment() const { return "NULL"; };
    virtual const double limit()     const { return 0; };
    virtual const mClock lifetime()  const { return 0; };
    virtual const string explain()   const = 0;
    virtual       string explainOK() const = 0;
    virtual       string explainKO() const { return ""; };
    const string explanation(const bool &loaded) const {
      string msg = loaded
        ? explainOK()
        : explainKO();
      size_t token = msg.find("%");
      if (token != string::npos)
        msg.replace(token, 1, explain());
      return msg;
    };
  };
  template <typename mData> struct mStructFromDb: public mFromDb {
    virtual const json blob() const {
      return *(mData*)this;
    };
    virtual const bool pull(const json &j) {
      if (j.empty()) return false;
      from_json(j.at(0), *(mData*)this);
      return true;
    };
    virtual string explainOK() const {
      return "loaded last % OK";
    };
  };
  template <typename mData> struct mVectorFromDb: public mFromDb {
    vector<mData> rows;
    typedef typename vector<mData>::reference                           reference;
    typedef typename vector<mData>::const_reference               const_reference;
    typedef typename vector<mData>::iterator                             iterator;
    typedef typename vector<mData>::const_iterator                 const_iterator;
    typedef typename vector<mData>::reverse_iterator             reverse_iterator;
    typedef typename vector<mData>::const_reverse_iterator const_reverse_iterator;
    iterator                 begin()       noexcept { return rows.begin(); };
    const_iterator           begin() const noexcept { return rows.begin(); };
    const_iterator          cbegin() const noexcept { return rows.cbegin(); };
    iterator                   end()       noexcept { return rows.end(); };
    const_iterator             end() const noexcept { return rows.end(); };
    reverse_iterator        rbegin()       noexcept { return rows.rbegin(); };
    const_reverse_iterator crbegin() const noexcept { return rows.crbegin(); };
    reverse_iterator          rend()       noexcept { return rows.rend(); };
    bool                     empty() const noexcept { return rows.empty(); };
    size_t                    size() const noexcept { return rows.size(); };
    reference                front()                { return rows.front(); };
    const_reference          front() const          { return rows.front(); };
    reference                 back()                { return rows.back(); };
    const_reference           back() const          { return rows.back(); };
    reference                   at(size_t n)        { return rows.at(n); };
    const_reference             at(size_t n) const  { return rows.at(n); };
    virtual void erase() {
      if (size() > limit())
        rows.erase(begin(), end() - limit());
    };
    virtual void push_back(const mData &row) {
      rows.push_back(row);
      push();
      erase();
    };
    virtual const bool pull(const json &j) {
      for (const json &it : j)
        rows.push_back(it);
      return !empty();
    };
    virtual const json blob() const {
      return back();
    };
    virtual const string explain() const {
      return to_string(size());
    };
  };

  struct mQuotingParams: public mStructFromDb<mQuotingParams>,
                         public mJsonToClient<mQuotingParams> {
    mPrice            widthPing                       = 2.0;
    double            widthPingPercentage             = 0.25;
    mPrice            widthPong                       = 2.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    mAmount           bestWidthSize                   = 0;
    mAmount           buySize                         = 0.02;
    unsigned int      buySizePercentage               = 7;
    bool              buySizeMax                      = false;
    mAmount           sellSize                        = 0.01;
    unsigned int      sellSizePercentage              = 7;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::Boomerang;
    unsigned int      bullets                         = 2;
    mPrice            range                           = 0.5;
    double            rangePercentage                 = 5.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    mAmount           targetBasePosition              = 1.0;
    unsigned int      targetBasePositionPercentage    = 50;
    mAmount           positionDivergence              = 0.9;
    mAmount           positionDivergenceMin           = 0.4;
    unsigned int      positionDivergencePercentage    = 21;
    unsigned int      positionDivergencePercentageMin = 10;
    mPDivMode         positionDivergenceMode          = mPDivMode::Manual;
    bool              percentageValues                = false;
    mAutoPositionMode autoPositionMode                = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing   = mAPR::Off;
    mSOP              superTrades                     = mSOP::Off;
    double            tradesPerMinute                 = 0.9;
    unsigned int      tradeRateSeconds                = 3;
    bool              protectionEwmaWidthPing         = false;
    bool              protectionEwmaQuotePrice        = true;
    unsigned int      protectionEwmaPeriods           = 200;
    mSTDEV            quotingStdevProtection          = mSTDEV::Off;
    bool              quotingStdevBollingerBands      = false;
    double            quotingStdevProtectionFactor    = 1.0;
    unsigned int      quotingStdevProtectionPeriods   = 1200;
    double            ewmaSensiblityPercentage        = 0.5;
    bool              quotingEwmaTrendProtection      = false;
    double            quotingEwmaTrendThreshold       = 2.0;
    unsigned int      veryLongEwmaPeriods             = 400;
    unsigned int      longEwmaPeriods                 = 200;
    unsigned int      mediumEwmaPeriods               = 100;
    unsigned int      shortEwmaPeriods                = 50;
    unsigned int      extraShortEwmaPeriods           = 12;
    unsigned int      ultraShortEwmaPeriods           = 3;
    double            aprMultiplier                   = 2;
    double            sopWidthMultiplier              = 2;
    double            sopSizeMultiplier               = 2;
    double            sopTradesMultiplier             = 2;
    bool              cancelOrdersAuto                = false;
    double            cleanPongsAuto                  = 0.0;
    double            profitHourInterval              = 0.5;
    bool              audio                           = false;
    unsigned int      delayUI                         = 7;
    bool              _diffVLEP                       = false;
    bool              _diffLEP                        = false;
    bool              _diffMEP                        = false;
    bool              _diffSEP                        = false;
    bool              _diffXSEP                       = false;
    bool              _diffUEP                        = false;
    void from_json(const json &j) {
      widthPing                       = fmax(1e-8,            j.value("widthPing", widthPing));
      widthPingPercentage             = fmin(1e+5, fmax(1e-4, j.value("widthPingPercentage", widthPingPercentage)));
      widthPong                       = fmax(1e-8,            j.value("widthPong", widthPong));
      widthPongPercentage             = fmin(1e+5, fmax(1e-4, j.value("widthPongPercentage", widthPongPercentage)));
      widthPercentage                 =                       j.value("widthPercentage", widthPercentage);
      bestWidth                       =                       j.value("bestWidth", bestWidth);
      bestWidthSize                   = fmax(0,               j.value("bestWidthSize", bestWidthSize));
      buySize                         = fmax(1e-8,            j.value("buySize", buySize));
      buySizePercentage               = fmin(1e+2, fmax(1,    j.value("buySizePercentage", buySizePercentage)));
      buySizeMax                      =                       j.value("buySizeMax", buySizeMax);
      sellSize                        = fmax(1e-8,            j.value("sellSize", sellSize));
      sellSizePercentage              = fmin(1e+2, fmax(1,    j.value("sellSizePercentage", sellSizePercentage)));
      sellSizeMax                     =                       j.value("sellSizeMax", sellSizeMax);
      pingAt                          =                       j.value("pingAt", pingAt);
      pongAt                          =                       j.value("pongAt", pongAt);
      mode                            =                       j.value("mode", mode);
      safety                          =                       j.value("safety", safety);
      bullets                         = fmin(10, fmax(1,      j.value("bullets", bullets)));
      range                           =                       j.value("range", range);
      rangePercentage                 = fmin(1e+2, fmax(1e-3, j.value("rangePercentage", rangePercentage)));
      fvModel                         =                       j.value("fvModel", fvModel);
      targetBasePosition              =                       j.value("targetBasePosition", targetBasePosition);
      targetBasePositionPercentage    = fmin(1e+2, fmax(0,    j.value("targetBasePositionPercentage", targetBasePositionPercentage)));
      positionDivergenceMin           =                       j.value("positionDivergenceMin", positionDivergenceMin);
      positionDivergenceMode          =                       j.value("positionDivergenceMode", positionDivergenceMode);
      positionDivergence              =                       j.value("positionDivergence", positionDivergence);
      positionDivergencePercentage    = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentage", positionDivergencePercentage)));
      positionDivergencePercentageMin = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentageMin", positionDivergencePercentageMin)));
      percentageValues                =                       j.value("percentageValues", percentageValues);
      autoPositionMode                =                       j.value("autoPositionMode", autoPositionMode);
      aggressivePositionRebalancing   =                       j.value("aggressivePositionRebalancing", aggressivePositionRebalancing);
      superTrades                     =                       j.value("superTrades", superTrades);
      tradesPerMinute                 =                       j.value("tradesPerMinute", tradesPerMinute);
      tradeRateSeconds                = fmax(0,               j.value("tradeRateSeconds", tradeRateSeconds));
      protectionEwmaWidthPing         =                       j.value("protectionEwmaWidthPing", protectionEwmaWidthPing);
      protectionEwmaQuotePrice        =                       j.value("protectionEwmaQuotePrice", protectionEwmaQuotePrice);
      protectionEwmaPeriods           = fmax(1,               j.value("protectionEwmaPeriods", protectionEwmaPeriods));
      quotingStdevProtection          =                       j.value("quotingStdevProtection", quotingStdevProtection);
      quotingStdevBollingerBands      =                       j.value("quotingStdevBollingerBands", quotingStdevBollingerBands);
      quotingStdevProtectionFactor    =                       j.value("quotingStdevProtectionFactor", quotingStdevProtectionFactor);
      quotingStdevProtectionPeriods   = fmax(1,               j.value("quotingStdevProtectionPeriods", quotingStdevProtectionPeriods));
      ewmaSensiblityPercentage        =                       j.value("ewmaSensiblityPercentage", ewmaSensiblityPercentage);
      quotingEwmaTrendProtection      =                       j.value("quotingEwmaTrendProtection", quotingEwmaTrendProtection);
      quotingEwmaTrendThreshold       =                       j.value("quotingEwmaTrendThreshold", quotingEwmaTrendThreshold);
      veryLongEwmaPeriods             = fmax(1,               j.value("veryLongEwmaPeriods", veryLongEwmaPeriods));
      longEwmaPeriods                 = fmax(1,               j.value("longEwmaPeriods", longEwmaPeriods));
      mediumEwmaPeriods               = fmax(1,               j.value("mediumEwmaPeriods", mediumEwmaPeriods));
      shortEwmaPeriods                = fmax(1,               j.value("shortEwmaPeriods", shortEwmaPeriods));
      extraShortEwmaPeriods           = fmax(1,               j.value("extraShortEwmaPeriods", extraShortEwmaPeriods));
      ultraShortEwmaPeriods           = fmax(1,               j.value("ultraShortEwmaPeriods", ultraShortEwmaPeriods));
      aprMultiplier                   =                       j.value("aprMultiplier", aprMultiplier);
      sopWidthMultiplier              =                       j.value("sopWidthMultiplier", sopWidthMultiplier);
      sopSizeMultiplier               =                       j.value("sopSizeMultiplier", sopSizeMultiplier);
      sopTradesMultiplier             =                       j.value("sopTradesMultiplier", sopTradesMultiplier);
      cancelOrdersAuto                =                       j.value("cancelOrdersAuto", cancelOrdersAuto);
      cleanPongsAuto                  =                       j.value("cleanPongsAuto", cleanPongsAuto);
      profitHourInterval              =                       j.value("profitHourInterval", profitHourInterval);
      audio                           =                       j.value("audio", audio);
      delayUI                         = fmax(0,               j.value("delayUI", delayUI));
      if (mode == mQuotingMode::Depth)
        widthPercentage = false;
    };
    void kiss(json *const j) {
      previous = {this};
      from_json(*j);
      diff(previous);
      push();
      send();
    };
    const mMatter about() const {
      return mMatter::QuotingParameters;
    };
    const string explain() const {
      return "Quoting Parameters";
    };
    string explainKO() const {
      return "using default values for %";
    };
    private:
      struct mPreviousQParams {
        unsigned int veryLongEwmaPeriods   = 0,
                     longEwmaPeriods       = 0,
                     mediumEwmaPeriods     = 0,
                     shortEwmaPeriods      = 0,
                     extraShortEwmaPeriods = 0,
                     ultraShortEwmaPeriods = 0;
        mPreviousQParams()
        {};
        mPreviousQParams(const mQuotingParams *const prev)
          : veryLongEwmaPeriods(  prev->veryLongEwmaPeriods  )
          , longEwmaPeriods(      prev->longEwmaPeriods      )
          , mediumEwmaPeriods(    prev->mediumEwmaPeriods    )
          , shortEwmaPeriods(     prev->shortEwmaPeriods     )
          , extraShortEwmaPeriods(prev->extraShortEwmaPeriods)
          , ultraShortEwmaPeriods(prev->ultraShortEwmaPeriods)
        {};
      } previous;
      void diff(const mPreviousQParams &prev) {
        _diffVLEP = prev.veryLongEwmaPeriods != veryLongEwmaPeriods;
        _diffLEP  = prev.longEwmaPeriods != longEwmaPeriods;
        _diffMEP  = prev.mediumEwmaPeriods != mediumEwmaPeriods;
        _diffSEP  = prev.shortEwmaPeriods != shortEwmaPeriods;
        _diffXSEP = prev.extraShortEwmaPeriods != extraShortEwmaPeriods;
        _diffUEP  = prev.ultraShortEwmaPeriods != ultraShortEwmaPeriods;
      };
  } qp;
  static void to_json(json &j, const mQuotingParams &k) {
    j = {
      {                      "widthPing", k.widthPing                      },
      {            "widthPingPercentage", k.widthPingPercentage            },
      {                      "widthPong", k.widthPong                      },
      {            "widthPongPercentage", k.widthPongPercentage            },
      {                "widthPercentage", k.widthPercentage                },
      {                      "bestWidth", k.bestWidth                      },
      {                  "bestWidthSize", k.bestWidthSize                  },
      {                        "buySize", k.buySize                        },
      {              "buySizePercentage", k.buySizePercentage              },
      {                     "buySizeMax", k.buySizeMax                     },
      {                       "sellSize", k.sellSize                       },
      {             "sellSizePercentage", k.sellSizePercentage             },
      {                    "sellSizeMax", k.sellSizeMax                    },
      {                         "pingAt", k.pingAt                         },
      {                         "pongAt", k.pongAt                         },
      {                           "mode", k.mode                           },
      {                         "safety", k.safety                         },
      {                        "bullets", k.bullets                        },
      {                          "range", k.range                          },
      {                "rangePercentage", k.rangePercentage                },
      {                        "fvModel", k.fvModel                        },
      {             "targetBasePosition", k.targetBasePosition             },
      {   "targetBasePositionPercentage", k.targetBasePositionPercentage   },
      {             "positionDivergence", k.positionDivergence             },
      {   "positionDivergencePercentage", k.positionDivergencePercentage   },
      {          "positionDivergenceMin", k.positionDivergenceMin          },
      {"positionDivergencePercentageMin", k.positionDivergencePercentageMin},
      {         "positionDivergenceMode", k.positionDivergenceMode         },
      {               "percentageValues", k.percentageValues               },
      {               "autoPositionMode", k.autoPositionMode               },
      {  "aggressivePositionRebalancing", k.aggressivePositionRebalancing  },
      {                    "superTrades", k.superTrades                    },
      {                "tradesPerMinute", k.tradesPerMinute                },
      {               "tradeRateSeconds", k.tradeRateSeconds               },
      {        "protectionEwmaWidthPing", k.protectionEwmaWidthPing        },
      {       "protectionEwmaQuotePrice", k.protectionEwmaQuotePrice       },
      {          "protectionEwmaPeriods", k.protectionEwmaPeriods          },
      {         "quotingStdevProtection", k.quotingStdevProtection         },
      {     "quotingStdevBollingerBands", k.quotingStdevBollingerBands     },
      {   "quotingStdevProtectionFactor", k.quotingStdevProtectionFactor   },
      {  "quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods  },
      {       "ewmaSensiblityPercentage", k.ewmaSensiblityPercentage       },
      {     "quotingEwmaTrendProtection", k.quotingEwmaTrendProtection     },
      {      "quotingEwmaTrendThreshold", k.quotingEwmaTrendThreshold      },
      {            "veryLongEwmaPeriods", k.veryLongEwmaPeriods            },
      {                "longEwmaPeriods", k.longEwmaPeriods                },
      {              "mediumEwmaPeriods", k.mediumEwmaPeriods              },
      {               "shortEwmaPeriods", k.shortEwmaPeriods               },
      {          "extraShortEwmaPeriods", k.extraShortEwmaPeriods          },
      {          "ultraShortEwmaPeriods", k.ultraShortEwmaPeriods          },
      {                  "aprMultiplier", k.aprMultiplier                  },
      {             "sopWidthMultiplier", k.sopWidthMultiplier             },
      {              "sopSizeMultiplier", k.sopSizeMultiplier              },
      {            "sopTradesMultiplier", k.sopTradesMultiplier            },
      {               "cancelOrdersAuto", k.cancelOrdersAuto               },
      {                 "cleanPongsAuto", k.cleanPongsAuto                 },
      {             "profitHourInterval", k.profitHourInterval             },
      {                          "audio", k.audio                          },
      {                        "delayUI", k.delayUI                        }
    };
  };
  static void from_json(const json &j, mQuotingParams &k) {
    k.from_json(j);
  };

  struct mLastOrder {
    mPrice  price          = 0;
    mAmount tradeQuantity  = 0;
    mSide   side           = (mSide)0;
    bool    isPong         = false;
    mLastOrder()
    {};
    mLastOrder(const mOrder *const order, const mOrder &raw)
      : price(        order ? order->price      : 0       )
      , tradeQuantity(order ? raw.tradeQuantity : 0       )
      , side(         order ? order->side       : (mSide)0)
      , isPong(       order ? order->isPong     : false   )
    {};
  };
  struct mOrders: public mToScreen,
                  public mJsonToClient<mOrders> {
    mLastOrder updated;
    private:
      unordered_map<mRandId, mOrder> orders;
    public:
      mOrder *const find(const mRandId &orderId) {
        return (orderId.empty()
          or orders.find(orderId) == orders.end()
        ) ? nullptr
          : &orders.at(orderId);
      };
      mOrder *const findsert(const mOrder &raw) {
        if (raw.status == mStatus::Waiting and !raw.orderId.empty())
          return &(orders[raw.orderId] = raw);
        if (raw.orderId.empty() and !raw.exchangeId.empty()) {
          unordered_map<mRandId, mOrder>::iterator it = find_if(
            orders.begin(), orders.end(),
            [&](const pair<mRandId, mOrder> &it_) {
              return raw.exchangeId == it_.second.exchangeId;
            }
          );
          if (it != orders.end())
            return &it->second;
        }
        return find(raw.orderId);
      };
      const double heldAmount(const mSide &side) const {
        double held = 0;
        for (const unordered_map<mRandId, mOrder>::value_type &it : orders)
          if (it.second.side == side)
            held += (
              it.second.side == mSide::Ask
                ? it.second.quantity
                : it.second.quantity * it.second.price
            );
        return held;
      };
      void resetFilters(
        unordered_map<mPrice, mAmount> *const filterBidOrders,
        unordered_map<mPrice, mAmount> *const filterAskOrders
      ) const {
        filterBidOrders->clear();
        filterAskOrders->clear();
        for (const unordered_map<mRandId, mOrder>::value_type &it : orders)
          (it.second.side == mSide::Bid
            ? *filterBidOrders
            : *filterAskOrders
          )[it.second.price] += it.second.quantity;
      };
      const vector<mOrder*> at(const mSide &side) {
        vector<mOrder*> sideOrders;
        for (unordered_map<mRandId, mOrder>::value_type &it : orders)
          if (side == it.second.side)
             sideOrders.push_back(&it.second);
        return sideOrders;
      };
      const vector<mOrder*> working() {
        vector<mOrder*> workingOrders;
        for (unordered_map<mRandId, mOrder>::value_type &it : orders)
          if (mStatus::Working == it.second.status
            and it.second.preferPostOnly
          ) workingOrders.push_back(&it.second);
        return workingOrders;
      };
      const vector<mOrder> working(const bool &sorted = false) const {
        vector<mOrder> workingOrders;
        for (const unordered_map<mRandId, mOrder>::value_type &it : orders)
          if (mStatus::Working == it.second.status)
            workingOrders.push_back(it.second);
        if (sorted)
          sort(workingOrders.begin(), workingOrders.end(),
            [](const mOrder &a, const mOrder &b) {
              return a.price > b.price;
            }
          );
        return workingOrders;
      };
      mOrder *const upsert(const mOrder &raw) {
        mOrder *const order = findsert(raw);
        mOrder::update(raw, order);
        if (debug()) {
          report(order, " saved ");
          report_size();
        }
        return order;
      };
      const bool replace(const mPrice &price, const bool &isPong, mOrder *const order) {
        const bool allowed = mOrder::replace(price, isPong, order);
        if (debug()) report(order, "replace");
        return allowed;
      };
      const bool cancel(mOrder *const order) {
        const bool allowed = mOrder::cancel(order);
        if (debug()) report(order, "cancel ");
        return allowed;
      };
      void purge(const mOrder *const order) {
        if (debug()) report(order, " purge ");
        orders.erase(order->orderId);
        if (debug()) report_size();
      };
      void read_from_gw(const mOrder &raw) {
        if (debug()) report(&raw, " reply ");
        mOrder *const order = upsert(raw);
        updated = {order, raw};
        if (!order) return;
        if (order->status == mStatus::Terminated)
          purge(order);
        send();
        refresh();
      };
      const mMatter about() const {
        return mMatter::OrderStatusReports;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const json blob() const {
        return working();
      };
    private:
      void report(const mOrder *const order, const string &reason) const {
        print("DEBUG OG", " " + reason + " " + (
          order
            ? order->orderId + "::" + order->exchangeId
              + " [" + to_string((int)order->status) + "]: "
              + str8(order->quantity) + " " + args->str("base") + " at price "
              + str8(order->price) + " " + args->str("quote")
            : "not found"
        ));
      };
      void report_size() const {
        print("DEBUG OG", "memory " + to_string(orders.size()));
      };
      const bool debug() const {
        return args->num("debug-orders");
      };
  };
  static void to_json(json &j, const mOrders &k) {
    j = k.blob();
  };

  struct mMarketTakers: public mJsonToClient<mTrade> {
    vector<mTrade> trades;
    mAmount        takersBuySize60s  = 0,
                   takersSellSize60s = 0;
    void timer_60s() {
      takersSellSize60s = takersBuySize60s = 0;
      if (trades.empty()) return;
      for (mTrade &it : trades)
        (it.side == mSide::Bid
          ? takersSellSize60s
          : takersBuySize60s
        ) += it.quantity;
      trades.clear();
    };
    void read_from_gw(const mTrade &raw) {
      trades.push_back(raw);
      send();
    };
    const mMatter about() const {
      return mMatter::MarketTrade;
    };
    const json blob() const {
      return trades.back();
    };
    const json hello() {
      return trades;
    };
  };
  static void to_json(json &j, const mMarketTakers &k) {
    j = k.trades;
  };

  struct mFairLevelsPrice: public mToScreen,
                           public mJsonToClient<mFairLevelsPrice> {
    private_ref:
      const mPrice &fairValue;
    public:
      mFairLevelsPrice(const mPrice &f)
        : fairValue(f)
      {};
      const mPrice currentPrice() const {
        return fairValue;
      };
      const mMatter about() const {
        return mMatter::FairValue;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      void send_refresh() {
        if (send()) refresh();
      };
      const bool send_same_blob() const {
        return false;
      };
      const bool send_asap() const {
        return false;
      };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", k.currentPrice()}
    };
  };

  struct mStdev {
    mPrice fv     = 0,
           topBid = 0,
           topAsk = 0;
    mStdev()
    {};
    mStdev(const mPrice &f, const mPrice &b, const mPrice &a)
      : fv(f)
      , topBid(b)
      , topAsk(a)
    {};
  };
  static void to_json(json &j, const mStdev &k) {
    j = {
      { "fv", k.fv    },
      {"bid", k.topBid},
      {"ask", k.topAsk}
    };
  };
  static void from_json(const json &j, mStdev &k) {
    k.fv = j.value("fv", 0.0);
    k.topBid = j.value("bid", 0.0);
    k.topAsk = j.value("ask", 0.0);
  };
  struct mStdevs: public mVectorFromDb<mStdev> {
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    private_ref:
      const mPrice &fairValue;
    public:
      mStdevs(const mPrice &f)
        : fairValue(f)
      {};
      const bool pull(const json &j) {
        const bool loaded = mVectorFromDb::pull(j);
        if (loaded) calc();
        return loaded;
      };
      void timer_1s(const mPrice &topBid, const mPrice &topAsk) {
        push_back(mStdev(fairValue, topBid, topAsk));
        calc();
      };
      void calc() {
        if (size() < 2) return;
        fair = calc(&fairMean, "fv");
        bid  = calc(&bidMean, "bid");
        ask  = calc(&askMean, "ask");
        top  = calc(&topMean, "top");
      };
      const mMatter about() const {
        return mMatter::STDEVStats;
      };
      const double limit() const {
        return qp.quotingStdevProtectionPeriods;
      };
      const mClock lifetime() const {
        return 1e+3 * limit();
      };
      string explainOK() const {
        return "loaded % STDEV Periods";
      };
    private:
      double calc(mPrice *const mean, const string &type) const {
        vector<mPrice> values;
        for (const mStdev &it : rows)
          if (type == "fv")
            values.push_back(it.fv);
          else if (type == "bid")
            values.push_back(it.topBid);
          else if (type == "ask")
            values.push_back(it.topAsk);
          else if (type == "top") {
            values.push_back(it.topBid);
            values.push_back(it.topAsk);
          }
        return calc(mean, qp.quotingStdevProtectionFactor, values);
      };
      double calc(mPrice *const mean, const double &factor, const vector<mPrice> &values) const {
        unsigned int n = values.size();
        if (!n) return 0;
        double sum = 0;
        for (const mPrice &it : values) sum += it;
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (const mPrice &it : values) {
          mPrice diff = it - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * factor;
      };
  };
  static void to_json(json &j, const mStdevs &k) {
    j = {
      {      "fv", k.fair    },
      {  "fvMean", k.fairMean},
      {    "tops", k.top     },
      {"topsMean", k.topMean },
      {     "bid", k.bid     },
      { "bidMean", k.bidMean },
      {     "ask", k.ask     },
      { "askMean", k.askMean }
    };
  };

  struct mFairHistory: public mVectorFromDb<mPrice> {
    const mMatter about() const {
      return mMatter::MarketDataLongTerm;
    };
    const double limit() const {
      return 5760;
    };
    const mClock lifetime() const {
      return 60e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Fair Values";
    };
  };

  struct mEwma: public mToScreen,
                public mStructFromDb<mEwma> {
    mFairHistory fairValue96h;
          mPrice mgEwmaVL = 0,
                 mgEwmaL  = 0,
                 mgEwmaM  = 0,
                 mgEwmaS  = 0,
                 mgEwmaXS = 0,
                 mgEwmaU  = 0,
                 mgEwmaP  = 0,
                 mgEwmaW  = 0;
          double mgEwmaTrendDiff              = 0,
                 targetPositionAutoPercentage = 0;
    private_ref:
      const mPrice &fairValue;
    public:
      mEwma(const mPrice &f)
        : fairValue(f)
      {};
      void timer_60s(const mPrice &averageWidth) {
        prepareHistory();
        calcProtections(averageWidth);
        calcPositions();
        calcTargetPositionAutoPercentage();
        push();
      };
      void calcFromHistory() {
        if (TRUEONCE(qp._diffVLEP)) calcFromHistory(&mgEwmaVL, qp.veryLongEwmaPeriods,   "VeryLong");
        if (TRUEONCE(qp._diffLEP))  calcFromHistory(&mgEwmaL,  qp.longEwmaPeriods,       "Long");
        if (TRUEONCE(qp._diffMEP))  calcFromHistory(&mgEwmaM,  qp.mediumEwmaPeriods,     "Medium");
        if (TRUEONCE(qp._diffSEP))  calcFromHistory(&mgEwmaS,  qp.shortEwmaPeriods,      "Short");
        if (TRUEONCE(qp._diffXSEP)) calcFromHistory(&mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
        if (TRUEONCE(qp._diffUEP))  calcFromHistory(&mgEwmaU,  qp.ultraShortEwmaPeriods, "UltraShort");
      };
      const mMatter about() const {
        return mMatter::EWMAStats;
      };
      const mClock lifetime() const {
        return 60e+3 * max(qp.veryLongEwmaPeriods,
                       max(qp.longEwmaPeriods,
                       max(qp.mediumEwmaPeriods,
                       max(qp.shortEwmaPeriods,
                       max(qp.extraShortEwmaPeriods,
                           qp.ultraShortEwmaPeriods
                       )))));
      };
      const string explain() const {
        return "EWMA Values";
      };
      string explainKO() const {
        return "consider to warm up some %";
      };
    private:
      void calc(mPrice *const mean, const unsigned int &periods, const mPrice &value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void prepareHistory() {
        fairValue96h.push_back(fairValue);
      };
      void calcFromHistory(mPrice *const mean, const unsigned int &periods, const string &name) {
        unsigned int n = fairValue96h.size();
        if (!n--) return;
        unsigned int x = 0;
        *mean = fairValue96h.front();
        while (n--) calc(mean, periods, fairValue96h.at(++x));
        print("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
      };
      void calcPositions() {
        calc(&mgEwmaVL, qp.veryLongEwmaPeriods,   fairValue);
        calc(&mgEwmaL,  qp.longEwmaPeriods,       fairValue);
        calc(&mgEwmaM,  qp.mediumEwmaPeriods,     fairValue);
        calc(&mgEwmaS,  qp.shortEwmaPeriods,      fairValue);
        calc(&mgEwmaXS, qp.extraShortEwmaPeriods, fairValue);
        calc(&mgEwmaU,  qp.ultraShortEwmaPeriods, fairValue);
        if (mgEwmaXS and mgEwmaU)
          mgEwmaTrendDiff = ((mgEwmaU * 1e+2) / mgEwmaXS) - 1e+2;
      };
      void calcProtections(const mPrice &averageWidth) {
        calc(&mgEwmaP, qp.protectionEwmaPeriods, fairValue);
        calc(&mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
      };
      void calcTargetPositionAutoPercentage() {
        unsigned int max3size = min((size_t)3, fairValue96h.size());
        mPrice SMA3 = accumulate(fairValue96h.end() - max3size, fairValue96h.end(), mPrice(),
          [](mPrice sma3, const mPrice &it) { return sma3 + it; }
        ) / max3size;
        double targetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 1e+2 / mgEwmaL) - 1e+2);
          double newEwmacrossing = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2);
          targetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          targetPosition = ((mgEwmaS * 1e+2 / mgEwmaL) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (mgEwmaL < mgEwmaVL) targetPosition = -1;
          else targetPosition = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        }
        targetPositionAutoPercentage = ((1 + max(-1.0, min(1.0, targetPosition))) / 2) * 1e+2;
      };
  };
  static void to_json(json &j, const mEwma &k) {
    j = {
      {  "ewmaVeryLong", k.mgEwmaVL       },
      {      "ewmaLong", k.mgEwmaL        },
      {    "ewmaMedium", k.mgEwmaM        },
      {     "ewmaShort", k.mgEwmaS        },
      {"ewmaExtraShort", k.mgEwmaXS       },
      {"ewmaUltraShort", k.mgEwmaU        },
      {     "ewmaQuote", k.mgEwmaP        },
      {     "ewmaWidth", k.mgEwmaW        },
      { "ewmaTrendDiff", k.mgEwmaTrendDiff}
    };
  };
  static void from_json(const json &j, mEwma &k) {
    k.mgEwmaVL = j.value("ewmaVeryLong", 0.0);
    k.mgEwmaL  = j.value("ewmaLong", 0.0);
    k.mgEwmaM  = j.value("ewmaMedium", 0.0);
    k.mgEwmaS  = j.value("ewmaShort", 0.0);
    k.mgEwmaXS = j.value("ewmaExtraShort", 0.0);
    k.mgEwmaU  = j.value("ewmaUltraShort", 0.0);
  };

  struct mMarketStats: public mJsonToClient<mMarketStats> {
               mEwma ewma;
             mStdevs stdev;
    mFairLevelsPrice fairPrice;
       mMarketTakers takerTrades;
    mMarketStats(const mPrice &f)
      : ewma(f)
      , stdev(f)
      , fairPrice(f)
    {};
    const mMatter about() const {
      return mMatter::MarketChart;
    };
    const bool realtime() const {
      return !qp.delayUI;
    };
  };
  static void to_json(json &j, const mMarketStats &k) {
    j = {
      {          "ewma", k.ewma                         },
      {    "stdevWidth", k.stdev                        },
      {     "fairValue", k.fairPrice.currentPrice()     },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
    };
  };

  struct mProduct: public mJsonToClient<mProduct> {
    const mPrice  *minTick = nullptr;
    const mAmount *minSize = nullptr;
    mProduct()
    {};
    const mMatter about() const {
      return mMatter::ProductAdvertisement;
    };
  };
  static void to_json(json &j, const mProduct &k) {
    j = {
      {   "exchange", args->str("exchange")                         },
      {       "base", args->str("base")                             },
      {      "quote", args->str("quote")                            },
      {    "minTick", *k.minTick                                    },
      {"environment", args->str("title")                            },
      { "matryoshka", args->str("matryoshka")                       },
      {   "homepage", "https://github.com/ctubio/Krypto-trading-bot"}
    };
  };

  struct mLevelsDiff: public mLevels,
                      public mJsonToClient<mLevelsDiff> {
      bool patched = false;
    private_ref:
      const mLevels &unfiltered;
    public:
      mLevelsDiff(const mLevels &u)
        : unfiltered(u)
      {};
      const bool empty() const {
        return patched
          ? bids.empty() and asks.empty()
          : mLevels::empty();
      };
      void send_patch() {
        if (ratelimit()) return;
        diff();
        if (!empty()) send_now();
        unfilter();
      };
      const mMatter about() const {
        return mMatter::MarketData;
      };
      const json hello() {
        unfilter();
        return mToClient::hello();
      };
    private:
      const bool ratelimit() {
        return unfiltered.empty() or empty()
          or !send_soon(qp.delayUI * 1e+3);
      };
      void unfilter() {
        bids = unfiltered.bids;
        asks = unfiltered.asks;
        patched = false;
      };
      void diff() {
        bids = diff(bids, unfiltered.bids);
        asks = diff(asks, unfiltered.asks);
        patched = true;
      };
      vector<mLevel> diff(const vector<mLevel> &from, vector<mLevel> to) const {
        vector<mLevel> patch;
        for (const mLevel &it : from) {
          vector<mLevel>::iterator it_ = find_if(
            to.begin(), to.end(),
            [&](const mLevel &_it) {
              return it.price == _it.price;
            }
          );
          mAmount size = 0;
          if (it_ != to.end()) {
            size = it_->size;
            to.erase(it_);
          }
          if (size != it.size)
            patch.push_back(mLevel(it.price, size));
        }
        if (!to.empty())
          patch.insert(patch.end(), to.begin(), to.end());
        return patch;
      };
  };
  static void to_json(json &j, const mLevelsDiff &k) {
    to_json(j, (mLevels)k);
    if (k.patched)
      j["diff"] = true;
  };
  struct mMarketLevels: public mLevels {
    unsigned int averageCount = 0;
          mPrice averageWidth = 0,
                 fairValue    = 0;
         mLevels unfiltered;
     mLevelsDiff diff;
    mMarketStats stats;
    private:
      unordered_map<mPrice, mAmount> filterBidOrders,
                                     filterAskOrders;
    private_ref:
      const mOrders  &orders;
      const mProduct &product;
    public:
      mMarketLevels(const mOrders &o, const mProduct &p)
        : diff(unfiltered)
        , stats(fairValue)
        , orders(o)
        , product(p)
      {};
      const bool warn_empty() const {
        const bool err = empty();
        if (err) stats.fairPrice.warn("QE", "Unable to calculate quote, missing market data");
        return err;
      };
      void timer_1s() {
        stats.stdev.timer_1s(bids.cbegin()->price, asks.cbegin()->price);
      };
      void timer_60s() {
        stats.takerTrades.timer_60s();
        stats.ewma.timer_60s(resetAverageWidth());
        stats.send();
      };
      const mPrice calcQuotesWidth(bool *const superSpread) const {
        const mPrice widthPing = fmax(
          qp.widthPercentage
            ? qp.widthPingPercentage * fairValue / 100
            : qp.widthPing,
          qp.protectionEwmaWidthPing and stats.ewma.mgEwmaW
            ? stats.ewma.mgEwmaW
            : 0
        );
        *superSpread = spread() > widthPing * qp.sopWidthMultiplier;
        return widthPing;
      };
      const bool ready() {
        filter();
        return !empty();
      };
      void read_from_gw(const mLevels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
        stats.fairPrice.send_refresh();
        diff.send_patch();
      };
    private:
      void filter() {
        orders.resetFilters(&filterBidOrders, &filterAskOrders);
        bids = filter(unfiltered.bids, &filterBidOrders);
        asks = filter(unfiltered.asks, &filterAskOrders);
        calcFairValue();
        calcAverageWidth();
      };
      void calcAverageWidth() {
        if (empty()) return;
        averageWidth = (
          (averageWidth * averageCount)
            + asks.cbegin()->price
            - bids.cbegin()->price
        );
        averageWidth /= ++averageCount;
      };
      const mPrice resetAverageWidth() {
        averageCount = 0;
        return averageWidth;
      };
      void calcFairValue() {
        if (empty())
          fairValue = 0;
        else if (qp.fvModel == mFairValueModel::BBO)
          fairValue = (asks.cbegin()->price
                     + bids.cbegin()->price) / 2;
        else if (qp.fvModel == mFairValueModel::wBBO)
          fairValue = (
            bids.cbegin()->price * bids.cbegin()->size
          + asks.cbegin()->price * asks.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        else
          fairValue = (
            bids.cbegin()->price * asks.cbegin()->size
          + asks.cbegin()->price * bids.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        if (fairValue)
          fairValue = ROUND(fairValue, *product.minTick);
      };
      const vector<mLevel> filter(vector<mLevel> levels, unordered_map<mPrice, mAmount> *const filterOrders) {
        if (!filterOrders->empty())
          for (vector<mLevel>::iterator it = levels.begin(); it != levels.end();) {
            for (unordered_map<mPrice, mAmount>::iterator it_ = filterOrders->begin(); it_ != filterOrders->end();)
              if (abs(it->price - it_->first) < *product.minTick) {
                it->size -= it_->second;
                filterOrders->erase(it_);
                break;
              } else ++it_;
            if (it->size < *product.minSize) it = levels.erase(it);
            else ++it;
            if (filterOrders->empty()) break;
          }
        return levels;
      };
  };

  struct mProfit {
    mAmount baseValue  = 0,
            quoteValue = 0;
     mClock time       = 0;
    mProfit()
    {};
    mProfit(mAmount b, mAmount q)
      : baseValue(b)
      , quoteValue(q)
      , time(Tstamp)
    {};
  };
  static void to_json(json &j, const mProfit &k) {
    j = {
      { "baseValue", k.baseValue },
      {"quoteValue", k.quoteValue},
      {      "time", k.time      }
    };
  };
  static void from_json(const json &j, mProfit &k) {
    k.baseValue  = j.value("baseValue", 0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time", (mClock)0);
  };
  struct mProfits: public mVectorFromDb<mProfit> {
    const bool ratelimit() const {
      return !empty() and crbegin()->time + 21e+3 > Tstamp;
    };
    const double calcBaseDiff() const {
      return calcDiffPercent(
        cbegin()->baseValue,
        crbegin()->baseValue
      );
    };
    const double calcQuoteDiff() const {
      return calcDiffPercent(
        cbegin()->quoteValue,
        crbegin()->quoteValue
      );
    };
    const double calcDiffPercent(mAmount older, mAmount newer) const {
      return ROUND(((newer - older) / newer) * 1e+2, 1e-2);
    };
    const mMatter about() const {
      return mMatter::Profit;
    };
    void erase() {
      for (vector<mProfit>::iterator it = begin(); it != end();)
        if (it->time + lifetime() > Tstamp) ++it;
        else it = rows.erase(it);
    };
    const double limit() const {
      return qp.profitHourInterval;
    };
    const mClock lifetime() const {
      return 3600e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Profits";
    };
  };

  struct mTradesHistory: public mToScreen,
                         public mVectorFromDb<mTrade>,
                         public mJsonToClient<mTrade> {
    void clearAll() {
      clear_if([](iterator it) {
        return true;
      });
    };
    void clearOne(const string &tradeId) {
      if (tradeId.empty()) return;
      clear_if([&tradeId](iterator it) {
        return it->tradeId == tradeId;
      }, true);
    };
    void clearClosed() {
      clear_if([](iterator it) {
        return it->Kqty >= it->quantity;
      });
    };
    void clearPongsAuto() {
      const mClock expire = Tstamp - (abs(qp.cleanPongsAuto) * 86400e3);
      const bool forcedClean = qp.cleanPongsAuto < 0;
      clear_if([&expire, &forcedClean](iterator it) {
        return (it->Ktime?:it->time) < expire and (
          forcedClean
          or it->Kqty >= it->quantity
        );
      });
    };
    void insert(const mLastOrder &order) {
      mAmount fee = 0;
      mTrade trade(
        order.price,
        order.tradeQuantity,
        order.side,
        order.isPong,
        Tstamp,
        abs(order.price * order.tradeQuantity),
        0, 0, 0, 0, 0, fee, false
      );
      print("GW " + args->str("exchange"), string(trade.isPong?"PONG":"PING") + " TRADE "
        + (trade.side == mSide::Bid ? "BUY  " : "SELL ")
        + str8(trade.quantity) + ' ' + args->str("base") + " at price "
        + str8(trade.price) + ' ' + args->str("quote") + " (value "
        + str8(trade.value) + ' ' + args->str("quote") + ")"
      );
      if (qp.safety == mQuotingSafety::Off or qp.safety == mQuotingSafety::PingPong)
        send_push_back(trade);
      else {
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * trade.price / 100
          : qp.widthPong;
        map<mPrice, string> matches;
        for (mTrade &it : rows)
          if (it.quantity - it.Kqty > 0
            and it.side != trade.side
            and (qp.pongAt == mPongAt::AveragePingFair
              or qp.pongAt == mPongAt::AveragePingAggressive
              or (trade.side == mSide::Bid
                ? (it.price > trade.price + widthPong)
                : (it.price < trade.price - widthPong)
              )
            )
          ) matches[it.price] = it.tradeId;
        matchPong(
          matches,
          trade,
          (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive)
            ? trade.side == mSide::Ask
            : trade.side == mSide::Bid
        );
      }
      if (qp.cleanPongsAuto)
        clearPongsAuto();
    };
    const mMatter about() const {
      return mMatter::Trades;
    };
    void erase() {
      if (crbegin()->Kqty < 0) rows.pop_back();
    };
    const json blob() const {
      if (crbegin()->Kqty == -1) return nullptr;
      else return mVectorFromDb::blob();
    };
    const string increment() const {
      return crbegin()->tradeId;
    };
    string explainOK() const {
      return "loaded % historical Trades";
    };
    const json hello() {
      for (mTrade &it : rows)
        it.loadedFromDB = true;
      return rows;
    };
    private:
      void clear_if(const function<const bool(iterator)> &fn, const bool &onlyOne = false) {
        for (iterator it = begin(); it != end();)
          if (fn(it)) {
            it->Kqty = -1;
            it = send_push_erase(it);
            if (onlyOne) break;
          } else ++it;
      };
      void matchPong(map<mPrice, string> matches, mTrade pong, bool reverse) {
        if (reverse) for (map<mPrice, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<mPrice, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (iterator it = begin(); it != end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->isPong = false;
            it->loadedFromDB = false;
            it = send_push_erase(it);
            break;
          }
          if (!eq) {
            send_push_back(pong);
          }
        }
      };
      bool matchPong(string match, mTrade* pong) {
        for (iterator it = begin(); it != end(); ++it) {
          if (it->tradeId != match) continue;
          mAmount Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
          it->Ktime = pong->time;
          it->Kprice = ((Kqty*pong->price) + (it->Kqty*it->Kprice)) / (it->Kqty+Kqty);
          it->Kqty = it->Kqty + Kqty;
          it->Kvalue = abs(it->Kqty*it->Kprice);
          pong->quantity = pong->quantity - Kqty;
          pong->value = abs(pong->price*pong->quantity);
          if (it->quantity<=it->Kqty)
            it->Kdiff = abs(it->quantity * it->price - it->Kqty * it->Kprice);
          it->isPong = true;
          it->loadedFromDB = false;
          it = send_push_erase(it);
          break;
        }
        return pong->quantity > 0;
      };
      void send_push_back(const mTrade &row) {
        rows.push_back(row);
        push();
        if (crbegin()->Kqty < 0) rbegin()->Kqty = -2;
        send();
      };
      iterator send_push_erase(iterator it) {
        mTrade row = *it;
        it = rows.erase(it);
        send_push_back(row);
        erase();
        return it;
      };
  };

  struct mRecentTrade {
     mPrice price    = 0;
    mAmount quantity = 0;
     mClock time     = 0;
    mRecentTrade(const mPrice &p, const mAmount &q)
      : price(p)
      , quantity(q)
      , time(Tstamp)
    {};
  };
  struct mRecentTrades {
    multimap<mPrice, mRecentTrade> buys,
                                   sells;
                           mAmount sumBuys       = 0,
                                   sumSells      = 0;
                            mPrice lastBuyPrice  = 0,
                                   lastSellPrice = 0;
    void insert(const mLastOrder &order) {
      (order.side == mSide::Bid
        ? lastBuyPrice
        : lastSellPrice
      ) = order.price;
      (order.side == mSide::Bid
        ? buys
        : sells
      ).insert(pair<mPrice, mRecentTrade>(
        order.price,
        mRecentTrade(order.price, order.tradeQuantity)
      ));
    };
    void expire() {
      if (buys.size()) expire(&buys);
      if (sells.size()) expire(&sells);
      skip();
      sumBuys = sum(&buys);
      sumSells = sum(&sells);
    };
    private:
      const mAmount sum(multimap<mPrice, mRecentTrade> *const k) const {
        mAmount sum = 0;
        for (multimap<mPrice, mRecentTrade>::value_type &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void expire(multimap<mPrice, mRecentTrade> *const k) {
        mClock now = Tstamp;
        for (multimap<mPrice, mRecentTrade>::iterator it = k->begin(); it != k->end();)
          if (it->second.time + qp.tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (buys.size() and sells.size()) {
          mRecentTrade &buy = buys.rbegin()->second;
          mRecentTrade &sell = sells.begin()->second;
          if (sell.price < buy.price) break;
          const mAmount buyQty = buy.quantity;
          buy.quantity -= sell.quantity;
          sell.quantity -= buyQty;
          if (buy.quantity <= 0)
            buys.erase(buys.rbegin()->first);
          if (sell.quantity <= 0)
            sells.erase(sells.begin()->first);
        }
      };
  };

  struct mSafety: public mJsonToClient<mSafety> {
              double buy      = 0,
                     sell     = 0,
                     combined = 0;
              mPrice buyPing  = 0,
                     sellPing = 0;
             mAmount buySize  = 0,
                     sellSize = 0;
       mRecentTrades recentTrades;
      mTradesHistory trades;
    private_ref:
      const mPrice           &fairValue;
      const mAmount          &baseValue,
                             &baseTotal,
                             &targetBasePosition;
    public:
      mSafety(const mPrice &f, const mAmount &v, const mAmount &t, const mAmount &p)
        : fairValue(f)
        , baseValue(v)
        , baseTotal(t)
        , targetBasePosition(p)
      {};
      void timer_1s() {
        calc();
      };
      void insertTrade(const mLastOrder &order) {
        recentTrades.insert(order);
        trades.insert(order);
        calc();
      };
      void calc() {
        if (!baseValue or !fairValue) return;
        calcSizes();
        calcPrices();
        recentTrades.expire();
        if (empty()) return;
        buy  = recentTrades.sumBuys / buySize;
        sell = recentTrades.sumSells / sellSize;
        combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
        send();
      };
      const bool empty() const {
        return !baseValue or !buySize or !sellSize;
      };
      const mMatter about() const {
        return mMatter::TradeSafetyValue;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcSizes() {
        sellSize = qp.percentageValues
            ? qp.sellSizePercentage * baseValue / 1e+2
            : qp.sellSize;
        buySize = qp.percentageValues
          ? qp.buySizePercentage * baseValue / 1e+2
          : qp.buySize;
        if (qp.aggressivePositionRebalancing == mAPR::Off) return;
        if (qp.buySizeMax)
          buySize = fmax(buySize, targetBasePosition - baseTotal);
        if (qp.sellSizeMax)
          sellSize = fmax(sellSize, baseTotal - targetBasePosition);
      };
      void calcPrices() {
        if (qp.safety == mQuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
          if (qp.safety == mQuotingSafety::Off) return;
          map<mPrice, mTrade> tradesBuy;
          map<mPrice, mTrade> tradesSell;
          for (const mTrade &it: trades)
            (it.side == mSide::Bid ? tradesBuy : tradesSell)[it.price] = it;
          mPrice widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * fairValue / 100
            : qp.widthPong;
          mAmount buyQty = 0,
                  sellQty = 0;
          if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
            matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
            matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
            if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
            if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
          } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
            matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
          } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
            matchAllPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchAllPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
          }
          if (buyQty) buyPing /= buyQty;
          if (sellQty) sellPing /= sellQty;
        }
      };
      void matchFirstPing(map<mPrice, mTrade> *tradesSide, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<mPrice, mTrade> *tradesSide, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, false, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<mPrice, mTrade> *tradesSide, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(false, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<mPrice, mTrade> *tradesSide, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width) {
        matchPing(false, false, tradesSide, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<mPrice, mTrade> *tradesSide, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<mPrice, mTrade>::reverse_iterator it = tradesSide->rbegin(); it != tradesSide->rend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<mPrice, mTrade>::iterator it = tradesSide->begin(); it != tradesSide->end(); ++it)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      const bool matchPing(bool _near, bool _far, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, mPrice fv, mPrice price, mAmount qtyTrade, mPrice priceTrade, mAmount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and KqtyTrade < qtyTrade
        ) {
          mAmount qty_ = qtyTrade;
          if (_near or _far)
            qty_ = fmin(qtyMax - *qty, qty_);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax and (_near or _far);
      };
  };
  static void to_json(json &j, const mSafety &k) {
    j = {
      {     "buy", k.buy     },
      {    "sell", k.sell    },
      {"combined", k.combined},
      { "buyPing", k.buyPing },
      {"sellPing", k.sellPing}
    };
  };

  struct mTarget: public mToScreen,
                  public mStructFromDb<mTarget>,
                  public mJsonToClient<mTarget> {
    mAmount targetBasePosition = 0,
            positionDivergence = 0;
    private_ref:
      const double  &targetPositionAutoPercentage;
      const mAmount &baseValue;
    public:
      mTarget(const double &t, const mAmount &v)
        : targetPositionAutoPercentage(t)
        , baseValue(v)
      {};
      void calcTargetBasePos() {
        if (warn_empty()) return;
        targetBasePosition = ROUND(qp.autoPositionMode == mAutoPositionMode::Manual
          ? (qp.percentageValues
            ? qp.targetBasePositionPercentage * baseValue / 1e+2
            : qp.targetBasePosition)
          : targetPositionAutoPercentage * baseValue / 1e+2
        , 1e-4);
        calcPDiv();
        if (send()) {
          push();
          if (debug()) report();
        }
      };
      const bool warn_empty() const {
        const bool err = empty();
        if (err) warn("PG", "Unable to calculate TBP, missing wallet data");
        return err;
      };
      const bool empty() const {
        return !baseValue;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const mMatter about() const {
        return mMatter::TargetBasePosition;
      };
      const string explain() const {
        return to_string(targetBasePosition);
      };
      string explainOK() const {
        return "loaded TBP = % " + args->str("base");
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcPDiv() {
        mAmount pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * baseValue / 1e+2
          : qp.positionDivergence;
        if (qp.autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp.positionDivergenceMode)
          positionDivergence = pDiv;
        else {
          mAmount pDivMin = qp.percentageValues
            ? qp.positionDivergencePercentageMin * baseValue / 1e+2
            : qp.positionDivergenceMin;
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          if (mPDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (mPDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (mPDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (mPDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
        positionDivergence = ROUND(positionDivergence, 1e-4);
      };
      void report() const {
        print("PG", "TBP: "
          + to_string((int)(targetBasePosition / baseValue * 1e+2)) + "% = " + str8(targetBasePosition)
          + " " + args->str("base") + ", pDiv: "
          + to_string((int)(positionDivergence / baseValue * 1e+2)) + "% = " + str8(positionDivergence)
          + " " + args->str("base"));
      };
      const bool debug() const {
        return args->num("debug-wallet");
      };
  };
  static void to_json(json &j, const mTarget &k) {
    j = {
      { "tbp", k.targetBasePosition},
      {"pDiv", k.positionDivergence}
    };
  };
  static void from_json(const json &j, mTarget &k) {
    k.targetBasePosition = j.value("tbp", 0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
  };

  struct mWalletPosition: public mWallets,
                          public mJsonToClient<mWalletPosition> {
     mTarget target;
     mSafety safety;
    mProfits profits;
    private_ref:
      const mOrders &orders;
      const mPrice  &fairValue;
    public:
      mWalletPosition(const mOrders &o, const double &t, const mPrice &f)
        : target(t, base.value)
        , safety(f, base.value, base.total, target.targetBasePosition)
        , orders(o)
        , fairValue(f)
      {};
      const bool ready() const {
        return !safety.empty();
      };
      void read_from_gw(const mWallets &raw) {
        if (raw.empty()) return;
        base.currency = raw.base.currency;
        quote.currency = raw.quote.currency;
        base.reset(raw.base.amount, raw.base.held);
        quote.reset(raw.quote.amount, raw.quote.held);
        calcFunds();
      };
      void calcFunds() {
        calcFundsSilently();
        send();
      };
      void calcFundsAfterOrder(const mLastOrder &order, bool *const askForFees) {
        if (!order.price) return;
        calcHeldAmount(order.side);
        calcFundsSilently();
        if (order.tradeQuantity) {
          safety.insertTrade(order);
          *askForFees = true;
        }
      };
      const mMatter about() const {
        return mMatter::Position;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const bool send_asap() const {
        return false;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcFundsSilently() {
        if (empty() or !fairValue) return;
        if (args->dec("wallet-limit")) calcMaxWallet();
        calcValues();
        calcProfits();
        target.calcTargetBasePos();
      };
      void calcHeldAmount(const mSide &side) {
        (side == mSide::Ask
          ? base
          : quote
        ).reset(orders.heldAmount(side));
      };
      void calcValues() {
        base.value = ROUND(quote.total / fairValue + base.total, 1e-8);
        quote.value = ROUND(base.total * fairValue + quote.total, 1e-8);
      };
      void calcProfits() {
        if (!profits.ratelimit())
          profits.push_back(mProfit(base.value, quote.value));
        base.profit  = profits.calcBaseDiff();
        quote.profit = profits.calcQuoteDiff();
      };
      void calcMaxWallet() {
        mAmount maxWallet = args->dec("wallet-limit");
        maxWallet -= quote.held / fairValue;
        if (maxWallet > 0 and quote.amount / fairValue > maxWallet) {
          quote.amount = maxWallet * fairValue;
          maxWallet = 0;
        } else maxWallet -= quote.amount / fairValue;
        maxWallet -= base.held;
        if (maxWallet > 0 and base.amount > maxWallet)
          base.amount = maxWallet;
      };
  };

  struct mButtonSubmitNewOrder: public mFromClient {
    void kiss(json *const j) {
      if (!j->is_object() or !j->value("price", 0.0) or !j->value("quantity", 0.0))
        *j = nullptr;
    };
    const mMatter about() const {
      return mMatter::SubmitNewOrder;
    };
  };
  struct mButtonCancelOrder: public mFromClient {
    void kiss(json *const j) {
      *j = (j->is_object() and !j->value("orderId", "").empty())
        ? j->at("orderId").get<mRandId>()
        : nullptr;
    };
    const mMatter about() const {
      return mMatter::CancelOrder;
    };
  };
  struct mButtonCancelAllOrders: public mFromClient {
    const mMatter about() const {
      return mMatter::CancelAllOrders;
    };
  };
  struct mButtonCleanAllClosedTrades: public mFromClient {
    const mMatter about() const {
      return mMatter::CleanAllClosedTrades;
    };
  };
  struct mButtonCleanAllTrades: public mFromClient {
    const mMatter about() const {
      return mMatter::CleanAllTrades;
    };
  };
  struct mButtonCleanTrade: public mFromClient {
    void kiss(json *const j) {
      *j = (j->is_object() and !j->value("tradeId", "").empty())
        ? j->at("tradeId").get<string>()
        : nullptr;
    };
    const mMatter about() const {
      return mMatter::CleanTrade;
    };
  };
  struct mNotepad: public mJsonToClient<mNotepad> {
    string content;
    void kiss(json *const j) {
      if (j->is_array() and j->size() and j->at(0).is_string())
        content = j->at(0);
    };
    const mMatter about() const {
      return mMatter::Notepad;
    };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mButtons {
    mNotepad                    notepad;
    mButtonSubmitNewOrder       submit;
    mButtonCancelOrder          cancel;
    mButtonCancelAllOrders      cancelAll;
    mButtonCleanAllClosedTrades cleanTradesClosed;
    mButtonCleanAllTrades       cleanTrades;
    mButtonCleanTrade           cleanTrade;
  };

  struct mSemaphore: public mToScreen,
                     public mJsonToClient<mSemaphore> {
    mConnectivity greenButton  = mConnectivity::Disconnected,
                  greenGateway = mConnectivity::Disconnected;
    private:
      mConnectivity adminAgreement = mConnectivity::Disconnected;
    public:
      void kiss(json *const j) {
        if (j->is_object()
          and j->at("agree").is_number()
          and j->at("agree").get<mConnectivity>() != adminAgreement
        ) toggle();
      };
      const bool paused() const {
        return !greenButton;
      };
      const bool offline() const {
        return !greenGateway;
      };
      void agree(const bool &agreement) {
        adminAgreement = (mConnectivity)agreement;
      };
      void toggle() {
        agree(!adminAgreement);
        send_refresh();
      };
      void read_from_gw(const mConnectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          send_refresh();
        }
      };
      const mMatter about() const {
        return mMatter::Connectivity;
      };
    private:
      void send_refresh() {
        const mConnectivity k = greenGateway * adminAgreement;
        if (greenButton != k) {
          greenButton = k;
          focus("GW " + args->str("exchange"), "Quoting state changed to",
            string(paused() ? "DIS" : "") + "CONNECTED");
        }
        send();
        refresh();
      };
  };
  static void to_json(json &j, const mSemaphore &k) {
    j = {
      { "agree", k.greenButton },
      {"online", k.greenGateway}
    };
  };

  struct mQuote: public mLevel {
    const mSide       side   = (mSide)0;
          mQuoteState state  = mQuoteState::MissingData;
          bool        isPong = false;
    mQuote(const mSide &s)
      : side(s)
    {};
    void skip() {
      size = 0;
    };
    void clear(const mQuoteState &reason) {
      mLevel::clear();
      state = reason;
    };
    virtual const bool deprecates(const mPrice&) const = 0;
    const bool checkCrossed(const mQuote &opposite) {
      if (empty()) return false;
      if (opposite.empty() or deprecates(opposite.price)) {
        state = mQuoteState::Live;
        return false;
      }
      state = mQuoteState::Crossed;
      return true;
    };
  };
  struct mQuoteBid: public mQuote {
    mQuoteBid()
      : mQuote(mSide::Bid)
    {};
    const bool deprecates(const mPrice &higher) const {
      return price < higher;
    };
  };
  struct mQuoteAsk: public mQuote {
    mQuoteAsk()
      : mQuote(mSide::Ask)
    {};
    const bool deprecates(const mPrice &lower) const {
      return price > lower;
    };
  };
  struct mQuotes: public mToScreen {
    mQuoteBid bid;
    mQuoteAsk ask;
         bool superSpread = false;
    mQuotes()
    {};
    void checkCrossedQuotes() {
      if ((unsigned int)bid.checkCrossed(ask)
        | (unsigned int)ask.checkCrossed(bid)
      ) warn("QE", "Crossed bid/ask quotes detected, that is.. unexpected");
    };
    void debug(const string &reason) {
      if (debug())
        print("DEBUG QE", reason);
    };
    void debuq(const string &step) {
      if (debug())
        print("DEBUG QE", "[" + step + "] "
          + to_string((int)bid.state) + ":"
          + to_string((int)ask.state) + " "
          + ((json)*this).dump()
        );
    };
    private:
      const bool debug() const {
        return args->num("debug-quotes");
      };
  };
  static void to_json(json &j, const mQuotes &k) {
    j = {
      {"bid", k.bid},
      {"ask", k.ask}
    };
  };

  struct mDummyMarketMaker: public mToScreen {
    private:
      void (*calcRawQuotesFromMarket)(
        const mMarketLevels&,
        const mPrice&,
        const mPrice&,
        const mAmount&,
        const mAmount&,
              mQuotes&
      ) = nullptr;
    private_ref:
      const mProduct        &product;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
            mQuotes         &quotes;
    public:
      mDummyMarketMaker(const mProduct &p, const mMarketLevels &l, const mWalletPosition &w, mQuotes &q)
        : product(p)
        , levels(l)
        , wallet(w)
        , quotes(q)
      {};
      void mode(const string &reason) {
        if (qp.mode == mQuotingMode::Top)              calcRawQuotesFromMarket = calcTopOfMarket;
        else if (qp.mode == mQuotingMode::Mid)         calcRawQuotesFromMarket = calcMidOfMarket;
        else if (qp.mode == mQuotingMode::Join)        calcRawQuotesFromMarket = calcJoinMarket;
        else if (qp.mode == mQuotingMode::InverseJoin) calcRawQuotesFromMarket = calcInverseJoinMarket;
        else if (qp.mode == mQuotingMode::InverseTop)  calcRawQuotesFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == mQuotingMode::HamelinRat)  calcRawQuotesFromMarket = calcColossusOfMarket;
        else if (qp.mode == mQuotingMode::Depth)       calcRawQuotesFromMarket = calcDepthOfMarket;
        else error("QE", "Invalid quoting mode " + reason + ", consider to remove the database file");
      };
      void calcRawQuotes() const  {
        calcRawQuotesFromMarket(
          levels,
          *product.minTick,
          levels.calcQuotesWidth(&quotes.superSpread),
          wallet.safety.buySize,
          wallet.safety.sellSize,
          quotes
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(mQuoteState::WidthMustBeSmaller);
          quotes.ask.clear(mQuoteState::WidthMustBeSmaller);
          warn("QP", "Negative price detected, widthPing must be smaller");
        }
      };
    private:
      static void quoteAtTopOfMarket(const mMarketLevels &levels, const mPrice &minTick, mQuotes &quotes) {
        const mLevel &topBid = levels.bids.begin()->size > minTick
          ? levels.bids.at(0)
          : levels.bids.at(levels.bids.size() > 1 ? 1 : 0);
        const mLevel &topAsk = levels.asks.begin()->size > minTick
          ? levels.asks.at(0)
          : levels.asks.at(levels.asks.size() > 1 ? 1 : 0);
        quotes.bid.price = topBid.price;
        quotes.ask.price = topAsk.price;
      };
      static void calcTopOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price + minTick);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price - minTick);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcMidOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quotes.bid.price = fmax(levels.fairValue - widthPing, 0);
        quotes.ask.price = levels.fairValue + widthPing;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcJoinMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseJoinMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        mPrice mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseTopOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        mPrice mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        quotes.bid.price = quotes.bid.price + minTick;
        quotes.ask.price = quotes.ask.price - minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcColossusOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.size = 0;
        quotes.ask.size = 0;
        mAmount depth = 0;
        for (const mLevel &it : levels.bids) {
          if (quotes.bid.size < it.size and it.price <= quotes.bid.price) {
            quotes.bid.size = it.size;
            quotes.bid.price = it.price;
          }
          depth += it.size;
          if (depth > qp.widthPing) break;
        }
        depth=0;
        for (const mLevel &it : levels.asks) {
          if (quotes.ask.size < it.size and it.price >= quotes.ask.price) {
            quotes.ask.size = it.size;
            quotes.ask.price = it.price;
          }
          depth += it.size;
          if (depth > qp.widthPing) break;
        }
        if (quotes.bid.size) quotes.bid.price += minTick;
        if (quotes.ask.size) quotes.ask.price -= minTick;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcDepthOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &depth,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &quotes
      ) {
        mPrice bidPx = levels.bids.cbegin()->price;
        mAmount bidDepth = 0;
        for (const mLevel &it : levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        mPrice askPx = levels.asks.cbegin()->price;
        mAmount askDepth = 0;
        for (const mLevel &it : levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        quotes.bid.price = bidPx;
        quotes.ask.price = askPx;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
  };

  struct mAntonioCalculon: public mJsonToClient<mAntonioCalculon> {
                  mQuotes quotes;
        mDummyMarketMaker dummyMM;
    vector<const mOrder*> zombies;
             unsigned int countWaiting = 0,
                          countWorking = 0,
                          AK47inc      = 0;
                   string sideAPR      = "Off";
    private_ref:
      const mProduct        &product;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
    public:
      mAntonioCalculon(const mProduct &p, const mMarketLevels &l, const mWalletPosition &w)
        : dummyMM(p, l, w, quotes)
        , product(p)
        , levels(l)
        , wallet(w)
      {};
      vector<const mOrder*> clear() {
        send();
        states(mQuoteState::MissingData);
        countWaiting =
        countWorking = 0;
        vector<const mOrder*> zombies_;
        zombies.swap(zombies_);
        return zombies_;
      };
      void offline() {
        states(mQuoteState::Disconnected);
      };
      void paused() {
        states(mQuoteState::DisabledQuotes);
      };
      void calcQuotes() {
        states(mQuoteState::UnknownHeld);
        dummyMM.calcRawQuotes();
        applyQuotingParameters();
      };
      const bool abandon(const mOrder &order, mQuote &quote, unsigned int &bullets) {
        if (stillAlive(order)) {
          if (abs(order.price - quote.price) < *product.minTick)
            quote.skip();
          else if (order.status == mStatus::Waiting) {
            if (qp.safety != mQuotingSafety::AK47
              or !--bullets
            ) quote.skip();
          } else if (qp.safety != mQuotingSafety::AK47
            or quote.deprecates(order.price)
          ) {
            if (args->num("lifetime") and order.time + args->num("lifetime") > Tstamp)
              quote.skip();
            else return true;
          }
        }
        return false;
      };
      const mMatter about() const {
        return mMatter::QuoteStatus;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void states(const mQuoteState &state) {
        quotes.bid.state =
        quotes.ask.state = state;
      };
      const bool stillAlive(const mOrder &order) {
        if (order.status == mStatus::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
          ++countWaiting;
        } else ++countWorking;
        return order.preferPostOnly;
      };
      void applyQuotingParameters() {
        quotes.debuq("?"); applySuperTrades();
        quotes.debuq("A"); applyEwmaProtection();
        quotes.debuq("B"); applyTotalBasePosition();
        quotes.debuq("C"); applyStdevProtection();
        quotes.debuq("D"); applyAggressivePositionRebalancing();
        quotes.debuq("E"); applyAK47Increment();
        quotes.debuq("F"); applyBestWidth();
        quotes.debuq("G"); applyTradesPerMinute();
        quotes.debuq("H"); applyRoundPrice();
        quotes.debuq("I"); applyRoundSize();
        quotes.debuq("J"); applyDepleted();
        quotes.debuq("K"); applyWaitingPing();
        quotes.debuq("L"); applyEwmaTrendProtection();
        quotes.debuq("!");
        quotes.debug("totals " + ("toAsk: " + to_string(wallet.base.total))
                               + ",toBid: " + to_string(wallet.quote.total / levels.fairValue));
        quotes.checkCrossedQuotes();
      };
      void applySuperTrades() {
        if (!quotes.superSpread
          or (qp.superTrades != mSOP::Size and qp.superTrades != mSOP::TradesSize)
        ) return;
        if (!qp.buySizeMax and !quotes.bid.empty())
          quotes.bid.size = fmin(
            qp.sopSizeMultiplier * quotes.bid.size,
            (wallet.quote.amount / levels.fairValue) / 2
          );
        if (!qp.sellSizeMax and !quotes.ask.empty())
          quotes.ask.size = fmin(
            qp.sopSizeMultiplier * quotes.ask.size,
            wallet.base.amount / 2
          );
      };
      void applyEwmaProtection() {
        if (!qp.protectionEwmaQuotePrice or !levels.stats.ewma.mgEwmaP) return;
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(levels.stats.ewma.mgEwmaP, quotes.ask.price);
        if (!quotes.bid.empty())
          quotes.bid.price = fmin(levels.stats.ewma.mgEwmaP, quotes.bid.price);
      };
      void applyTotalBasePosition() {
        if (wallet.base.total < wallet.target.targetBasePosition - wallet.target.positionDivergence) {
          quotes.ask.clear(mQuoteState::TBPHeld);
          if (!quotes.bid.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Buy";
            if (!qp.buySizeMax)
              quotes.bid.size = fmin(
                qp.aprMultiplier * quotes.bid.size,
                wallet.target.targetBasePosition - wallet.base.total
              );
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          quotes.bid.clear(mQuoteState::TBPHeld);
          if (!quotes.ask.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Sell";
            if (!qp.sellSizeMax)
              quotes.ask.size = fmin(
                qp.aprMultiplier * quotes.ask.size,
                wallet.base.total - wallet.target.targetBasePosition
              );
          }
        }
        else sideAPR = "Off";
      };
      void applyStdevProtection() {
        if (qp.quotingStdevProtection == mSTDEV::Off or !levels.stats.stdev.fair) return;
        if (!quotes.ask.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Sell"
        ))
          quotes.ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.askMean )
              : levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.ask )),
            quotes.ask.price
          );
        if (!quotes.bid.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Buy"
        ))
          quotes.bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.bidMean )
              : levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.bid )),
            quotes.bid.price
          );
      };
      void applyAggressivePositionRebalancing() {
        if (qp.safety == mQuotingSafety::Off) return;
        const mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        const mPrice &safetyBuyPing = wallet.safety.buyPing;
        if (!quotes.ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Sell")
            or (qp.safety == mQuotingSafety::PingPong
              ? quotes.ask.price < safetyBuyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.ask.price = safetyBuyPing + widthPong;
          quotes.ask.isPong = quotes.ask.price >= safetyBuyPing + widthPong;
        }
        const mPrice &safetysellPing = wallet.safety.sellPing;
        if (!quotes.bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Buy")
            or (qp.safety == mQuotingSafety::PingPong
              ? quotes.bid.price > safetysellPing - widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.bid.price = safetysellPing - widthPong;
          quotes.bid.isPong = quotes.bid.price <= safetysellPing - widthPong;
        }
      };
      void applyAK47Increment() {
        if (qp.safety != mQuotingSafety::AK47) return;
        const mPrice range = qp.percentageValues
          ? qp.rangePercentage * wallet.base.value / 100
          : qp.range;
        if (!quotes.bid.empty())
          quotes.bid.price -= AK47inc * range;
        if (!quotes.ask.empty())
          quotes.ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyBestWidth() {
        if (!qp.bestWidth) return;
        const mAmount bestWidthSize = (sideAPR=="Off" ? qp.bestWidthSize : 0);
        mAmount depth = 0;
        if (!quotes.ask.empty())
          for (const mLevel &it : levels.asks)
            if (it.price > quotes.ask.price) {
              depth += it.size;
              if (depth < bestWidthSize) continue;
              const mPrice bestAsk = it.price - *product.minTick;
              if (bestAsk > levels.fairValue) {
                quotes.ask.price = bestAsk;
                break;
              }
            }
        depth = 0;
        if (!quotes.bid.empty())
          for (const mLevel &it : levels.bids)
            if (it.price < quotes.bid.price) {
              depth += it.size;
              if (depth < bestWidthSize) continue;
              const mPrice bestBid = it.price + *product.minTick;
              if (bestBid < levels.fairValue) {
                quotes.bid.price = bestBid;
                break;
              }
            }
      };
      void applyTradesPerMinute() {
        const double factor = (quotes.superSpread and (
          qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize
        )) ? qp.sopWidthMultiplier
           : 1;
        if (wallet.safety.sell >= qp.tradesPerMinute * factor)
          quotes.ask.clear(mQuoteState::MaxTradesSeconds);
        if (wallet.safety.buy >= qp.tradesPerMinute * factor)
          quotes.bid.clear(mQuoteState::MaxTradesSeconds);
      };
      void applyRoundPrice() {
        if (!quotes.bid.empty())
          quotes.bid.price = fmax(
            0,
            floor(quotes.bid.price / *product.minTick) * *product.minTick
          );
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(
            quotes.bid.price + *product.minTick,
            ceil(quotes.ask.price / *product.minTick) * *product.minTick
          );
      };
      void applyRoundSize() {
        if (!quotes.ask.empty())
          quotes.ask.size = floor(fmax(
            fmin(
              quotes.ask.size,
              wallet.base.total
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
        if (!quotes.bid.empty())
          quotes.bid.size = floor(fmax(
            fmin(
              quotes.bid.size,
              wallet.quote.total / levels.fairValue
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted() {
        if (quotes.bid.size > wallet.quote.total / levels.fairValue)
          quotes.bid.clear(mQuoteState::DepletedFunds);
        if (quotes.ask.size > wallet.base.total)
          quotes.ask.clear(mQuoteState::DepletedFunds);
      };
      void applyWaitingPing() {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!quotes.ask.isPong and (
          (quotes.bid.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) quotes.ask.clear(mQuoteState::WaitingPing);
        if (!quotes.bid.isPong and (
          (quotes.ask.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) quotes.bid.clear(mQuoteState::WaitingPing);
      };
      void applyEwmaTrendProtection() {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold)
          quotes.ask.clear(mQuoteState::UpTrendHeld);
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold)
          quotes.bid.clear(mQuoteState::DownTrendHeld);
      };
  };
  static void to_json(json &j, const mAntonioCalculon &k) {
    j = {
      {            "bidStatus", k.quotes.bid.state},
      {            "askStatus", k.quotes.ask.state},
      {              "sideAPR", k.sideAPR         },
      {"quotesInMemoryWaiting", k.countWaiting    },
      {"quotesInMemoryWorking", k.countWorking    },
      {"quotesInMemoryZombies", k.zombies.size()  }
    };
  };

  struct mBroker {
          mSemaphore semaphore;
    mAntonioCalculon calculon;
    private_ref:
      mOrders &orders;
    public:
      mBroker(mOrders &o, const mProduct &p, const mMarketLevels &l, const mWalletPosition &w)
        : calculon(p, l, w)
        , orders(o)
      {};
      const bool ready() {
        if (semaphore.offline()) {
          calculon.offline();
          return false;
        }
        return true;
      };
      const bool calcQuotes() {
        if (semaphore.paused()) {
          calculon.paused();
          return false;
        }
        calculon.calcQuotes();
        return true;
      };
      const vector<mOrder*> abandon(mQuote &quote) {
        vector<mOrder*> abandoned;
        unsigned int bullets = qp.bullets;
        const bool all = quote.state != mQuoteState::Live;
        for (mOrder *const it : orders.at(quote.side))
          if (all or calculon.abandon(*it, quote, bullets))
            abandoned.push_back(it);
        return abandoned;
      };
      void clear() {
        for (const mOrder *const it : calculon.clear())
          orders.purge(it);
      };
  };

  struct mMonitor: public mJsonToClient<mMonitor> {
    unsigned int /* ( | L | ) */ /* more */ orders_60s /* ? */;
    const string /*  )| O |(  */  * unlock;
        mProduct /* ( | C | ) */ /* this */ product;
                 /*  )| K |(  */ /* thanks! <3 */
    mMonitor()
      : orders_60s(0)
      , unlock(nullptr)
    {};
    const unsigned int memSize() const {
#ifdef _WIN32
      return 0;
#else
      struct rusage ru;
      return getrusage(RUSAGE_SELF, &ru) ? 0 : ru.ru_maxrss * 1e+3;
#endif
    };
    const unsigned int dbSize() const {
      if (args->str("database") == ":memory:") return 0;
      struct stat st;
      return stat(args->str("database").data(), &st) ? 0 : st.st_size;
    };
    void tick_orders() {
      orders_60s++;
    };
    void timer_60s() {
      send();
      orders_60s = 0;
    };
    const mMatter about() const {
      return mMatter::ApplicationState;
    };
  };
  static void to_json(json &j, const mMonitor &k) {
    j = {
      {     "a", *k.unlock                                         },
      {  "inet", string(mREST::inet ?: "")                         },
      {  "freq", k.orders_60s                                      },
      { "theme", args->num("ignore-moon") + args->num("ignore-sun")},
      {"memory", k.memSize()                                       },
      {"dbsize", k.dbSize()                                        }
    };
  };
}

#endif
