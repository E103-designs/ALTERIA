static void modeGeiger(unsigned long t) {
  static uint16_t activity, radA, radB;
  static unsigned long lastTick, lastNoise;
  static int lastA7, avgA7;

  if (modeJustChanged) {
    activity = 1200;
    radA = 0;
    radB = 0;
    lastTick = t;
    lastNoise = t;
    lastA7 = analogRead(RANDOM_SEED_PIN);
    avgA7 = lastA7;
    schClearAll();
    dacSetA(0);
    dacSetB(0);
    setGate(false);
  }

  if (checkReset()) {
    activity = 4095;
    radA = 4095;
    radB = 3200;
  }

  if ((unsigned long)(t - lastNoise) >= 40) {
    lastNoise = t;
    int n = analogRead(RANDOM_SEED_PIN);
    int diff = abs(n - lastA7);
    lastA7 = n;

    avgA7 = (avgA7 * 7 + n) / 8;
    int dev = abs(n - avgA7);

    int diffTh = map(ctrl3Val, 0, 1023, 22, 3);
    int devTh = map(ctrl3Val, 0, 1023, 26, 4);
    int boostScale = map(ctrl3Val, 0, 1023, 8, 32);

    if (diff > diffTh || dev > devTh) {
      uint16_t boost = (uint16_t)min(1800, (diff + dev) * boostScale);
      uint32_t a = (uint32_t)activity + boost;
      activity = (a > 4095UL) ? 4095 : (uint16_t)a;

      if ((diff + dev) > (diffTh + devTh)) {
        if (radA < 2000) radA = 2000;
        if (radB < 1600) radB = 1600;
      }
    }

    randomSeed((unsigned long)(n + micros() + ((unsigned long)activity << 1)));
  }

  unsigned long dt = t - lastTick;
  if (dt > 60) dt = 60;
  if (dt) {
    lastTick = t;

    uint16_t density = (uint16_t)map(ctrl1Val, 0, 1023, 1, 22);
    uint16_t persistence = (uint16_t)map(ctrl2Val, 0, 1023, 1, 60);

    uint16_t actDecay = (persistence >= 55) ? 1 : (uint16_t)max(1, 18 - (int)(persistence / 3));
    uint16_t dA = (persistence >= 55) ? 1 : (uint16_t)max(1, 16 - (int)(persistence / 3));
    uint16_t dB = (persistence >= 55) ? 1 : (uint16_t)max(1, 18 - (int)(persistence / 3));

    bool slowTail = (persistence > 40);

    for (unsigned long i = 0; i < dt; i++) {
      uint16_t actD = slowTail && (activity > 1200) ? 1 : actDecay;
      activity = (activity > actD) ? (activity - actD) : 0;

      uint16_t da = slowTail && (radA > 1500) ? 1 : dA;
      uint16_t db = slowTail && (radB > 1200) ? 1 : dB;

      radA = (radA > da) ? (radA - da) : 0;
      radB = (radB > db) ? (radB - db) : 0;

      uint16_t chance = (uint16_t)(((uint32_t)activity * density) / 4095UL);
      if (activity > 120) chance += 1;

      if (random(0, 1000) < chance) {
        unsigned long pw = 2 + (unsigned long)random(0, 10);
        schPulseG(t, 0, pw);

        uint16_t spikeA = (uint16_t)random(700, 4096);
        uint16_t spikeB = (uint16_t)random(200, 4096);

        if (spikeA > radA) radA = spikeA;
        else {
          uint32_t tmp = (uint32_t)radA + (spikeA / 2U);
          radA = (tmp > 4095UL) ? 4095 : (uint16_t)tmp;
        }

        if (random(0, 100) < 70) radB = spikeB;
        else {
          uint32_t tmp = (uint32_t)radB + (spikeB / 3U);
          radB = (tmp > 4095UL) ? 4095 : (uint16_t)tmp;
        }

        uint32_t aa = (uint32_t)activity + 40;
        activity = (aa > 4095UL) ? 4095 : (uint16_t)aa;
      }
    }

    dacSetA(radA);
    dacSetB(radB);
  }

  schService(t);
}
