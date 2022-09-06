/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testaddon.h"
#include "../../src/addons/addon.h"
#include "../../src/addons/addonguide.h"
#include "../../src/addons/addonmessage.h"
#include "../../src/addons/addonproperty.h"
#include "../../src/addons/addonpropertylist.h"
#include "../../src/addons/addontutorial.h"
#include "../../src/addons/conditionwatchers/addonconditionwatcherfeaturesenabled.h"
#include "../../src/addons/conditionwatchers/addonconditionwatchergroup.h"
#include "../../src/addons/conditionwatchers/addonconditionwatcherlocales.h"
#include "../../src/addons/conditionwatchers/addonconditionwatcherjavascript.h"
#include "../../src/addons/conditionwatchers/addonconditionwatchertimeend.h"
#include "../../src/addons/conditionwatchers/addonconditionwatchertimestart.h"
#include "../../src/addons/conditionwatchers/addonconditionwatchertriggertimesecs.h"
#include "../../src/localizer.h"
#include "../../src/models/feature.h"
#include "../../src/models/featuremodel.h"
#include "../../src/settingsholder.h"
#include "../../src/qmlengineholder.h"
#include "../../src/tutorial/tutorial.h"
#include "helper.h"

#include <QQmlApplicationEngine>

void TestAddon::property() {
  AddonProperty p;
  p.initialize("foo", "bar");
  QCOMPARE(p.get(), "bar");
  QCOMPARE(p.property("value").toString(), "bar");
}

void TestAddon::property_list() {
  AddonPropertyList p;
  p.append("a", "foo");
  p.append("b", "bar");

  QStringList list{"foo", "bar"};
  QCOMPARE(p.get(), list);
  QCOMPARE(p.property("value").toStringList(), list);
}

void TestAddon::conditions_data() {
  QTest::addColumn<QJsonObject>("conditions");
  QTest::addColumn<bool>("result");
  QTest::addColumn<QString>("settingKey");
  QTest::addColumn<QVariant>("settingValue");

  QTest::addRow("empty") << QJsonObject() << true << "" << QVariant();

  {
    QJsonObject obj;
    obj["platforms"] = QJsonArray{"foo"};
    QTest::addRow("platforms") << obj << false << "" << QVariant();
  }

  {
    QJsonObject obj;
    obj["enabled_features"] = QJsonArray{"appReview"};
    QTest::addRow("enabled_features") << obj << true << "" << QVariant();
  }

  {
    QJsonObject obj;
    QJsonObject settings;
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("empty settings") << obj << false << "" << QVariant();

    settings["op"] = "eq";
    settings["setting"] = "foo";
    settings["value"] = true;
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid settings") << obj << false << "" << QVariant();

    QTest::addRow("string to boolean type settings - boolean")
        << obj << true << "foo" << QVariant("wow");

    QTest::addRow("op=eq settings - boolean")
        << obj << true << "foo" << QVariant(true);

    QTest::addRow("op=eq settings - boolean 2")
        << obj << false << "foo" << QVariant(false);

    settings["op"] = "WOW";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid op settings - boolean")
        << obj << false << "foo" << QVariant(false);

    settings["op"] = "neq";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("op=neq settings - boolean")
        << obj << true << "foo" << QVariant(false);

    settings["op"] = "eq";
    settings["value"] = 42;
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid type settings - double")
        << obj << false << "foo" << QVariant("wow");

    QTest::addRow("op=eq settings - double")
        << obj << true << "foo" << QVariant(42);

    QTest::addRow("op=eq settings - double 2")
        << obj << false << "foo" << QVariant(43);

    settings["op"] = "WOW";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid op settings - double")
        << obj << false << "foo" << QVariant(43);

    settings["op"] = "neq";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("op=neq settings - double")
        << obj << true << "foo" << QVariant(43);

    settings["op"] = "eq";
    settings["value"] = "wow";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid type settings - string")
        << obj << false << "foo" << QVariant(false);

    QTest::addRow("op=eq settings - string")
        << obj << true << "foo" << QVariant("wow");

    QTest::addRow("op=eq settings - string 2")
        << obj << false << "foo" << QVariant("wooow");

    settings["op"] = "WOW";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("invalid op settings - string")
        << obj << false << "foo" << QVariant("woow");

    settings["op"] = "neq";
    obj["settings"] = QJsonArray{settings};
    QTest::addRow("op=neq settings - string")
        << obj << true << "foo" << QVariant("woow");
  }

  {
    QJsonObject obj;
    obj["min_client_version"] = "2.1";
    QTest::addRow("min client version ok")
        << obj << true << "" << QVariant("woow");
  }
  {
    QJsonObject obj;
    obj["min_client_version"] = "3.0";
    QTest::addRow("min client version ko")
        << obj << false << "" << QVariant("woow");
  }
  {
    QJsonObject obj;
    obj["max_client_version"] = "2.0";
    QTest::addRow("max client version ko")
        << obj << false << "" << QVariant("woow");
  }
  {
    QJsonObject obj;
    obj["max_client_version"] = "3.0";
    QTest::addRow("max client version ok")
        << obj << true << "" << QVariant("woow");
  }

  {
    QJsonObject obj;
    obj["trigger_time"] = 1;
    QTest::addRow("trigger time") << obj << true << "" << QVariant("woow");
  }

  // All of these conditions are not considered in `evaluteConditions` becaue
  // they are dynamic.
  {
    QJsonObject obj;
    obj["start_time"] = 1;
    QTest::addRow("start time (valid)")
        << obj << true << "" << QVariant("woow");
  }
  {
    QJsonObject obj;
    obj["start_time"] = QDateTime::currentSecsSinceEpoch() + 1;
    QTest::addRow("start time (expired)")
        << obj << true << "" << QVariant("woow");
  }

  {
    QJsonObject obj;
    obj["end_time"] = QDateTime::currentSecsSinceEpoch() + 1;
    QTest::addRow("end time (valid)") << obj << true << "" << QVariant("woow");
  }
  {
    QJsonObject obj;
    obj["end_time"] = QDateTime::currentSecsSinceEpoch() - 1;
    QTest::addRow("end time (expired)")
        << obj << true << "" << QVariant("woow");
  }
}

void TestAddon::conditions() {
  SettingsHolder settingsHolder;

  QFETCH(QJsonObject, conditions);
  QFETCH(bool, result);
  QFETCH(QString, settingKey);
  QFETCH(QVariant, settingValue);

  if (!settingKey.isEmpty()) {
    settingsHolder.setRawSetting(settingKey, settingValue);
  }

  QCOMPARE(Addon::evaluateConditions(conditions), result);
}

void TestAddon::conditionWatcher_javascript() {
  MozillaVPN vpn;

  QQmlApplicationEngine engine;
  QmlEngineHolder qml(&engine);
  SettingsHolder settingsHolder;

  QJsonObject content;
  content["id"] = "foo";
  content["blocks"] = QJsonArray();

  QJsonObject obj;
  obj["message"] = content;

  QObject parent;
  Addon* message = AddonMessage::create(&parent, "foo", "bar", "name", obj);

  QVERIFY(!AddonConditionWatcherJavascript::maybeCreate(message, QString()));
  QVERIFY(!AddonConditionWatcherJavascript::maybeCreate(message, "foo"));
  QVERIFY(!AddonConditionWatcherJavascript::maybeCreate(
      message, ":/addons_test/condition1.js"));

  {
    AddonConditionWatcher* a = AddonConditionWatcherJavascript::maybeCreate(
        message, ":/addons_test/condition2.js");
    QVERIFY(!!a);
    QVERIFY(!a->conditionApplied());
  }

  {
    AddonConditionWatcher* a = AddonConditionWatcherJavascript::maybeCreate(
        message, ":/addons_test/condition3.js");
    QVERIFY(!!a);
    QVERIFY(a->conditionApplied());
  }

  {
    AddonConditionWatcher* a = AddonConditionWatcherJavascript::maybeCreate(
        message, ":/addons_test/condition4.js");
    QVERIFY(!!a);
    QVERIFY(!a->conditionApplied());

    QEventLoop loop;
    bool currentStatus = false;
    connect(a, &AddonConditionWatcher::conditionChanged, [&](bool status) {
      currentStatus = status;
      loop.exit();
    });
    loop.exec();
    QVERIFY(currentStatus);
    loop.exec();
    QVERIFY(!currentStatus);
  }

  {
    AddonConditionWatcher* a = AddonConditionWatcherJavascript::maybeCreate(
        message, ":/addons_test/condition5.js");
    QVERIFY(!!a);
    QVERIFY(!a->conditionApplied());

    settingsHolder.setStartAtBoot(true);
    QVERIFY(a->conditionApplied());
  }
}

void TestAddon::conditionWatcher_locale() {
  SettingsHolder settingsHolder;

  QObject parent;

  // No locales -> no watcher.
  QVERIFY(!AddonConditionWatcherLocales::maybeCreate(&parent, QStringList()));

  AddonConditionWatcher* acw =
      AddonConditionWatcherLocales::maybeCreate(&parent, QStringList{"it"});
  QVERIFY(!!acw);

  QSignalSpy signalSpy(acw, &AddonConditionWatcher::conditionChanged);
  QCOMPARE(signalSpy.count(), 0);
  settingsHolder.setLanguageCode("en");
  QCOMPARE(signalSpy.count(), 0);
  settingsHolder.setLanguageCode("it_RU");
  QCOMPARE(signalSpy.count(), 1);
  settingsHolder.setLanguageCode("it");
  QCOMPARE(signalSpy.count(), 1);
  settingsHolder.setLanguageCode("es");
  QCOMPARE(signalSpy.count(), 2);

  settingsHolder.setLanguageCode("en");
  QVERIFY(!acw->conditionApplied());

  settingsHolder.setLanguageCode("it");
  QVERIFY(acw->conditionApplied());

  settingsHolder.setLanguageCode("ru");
  QVERIFY(!acw->conditionApplied());

  settingsHolder.setLanguageCode("it-IT");
  QVERIFY(acw->conditionApplied());

  settingsHolder.setLanguageCode("en");
  QVERIFY(!acw->conditionApplied());

  settingsHolder.setLanguageCode("it_RU");
  QVERIFY(acw->conditionApplied());
}

void TestAddon::conditionWatcher_featuresEnabled() {
  SettingsHolder settingsHolder;

  QObject parent;

  // Empty feature list
  QVERIFY(!AddonConditionWatcherFeaturesEnabled::maybeCreate(&parent,
                                                             QStringList()));

  // Invalid feature list
  QVERIFY(!AddonConditionWatcherFeaturesEnabled::maybeCreate(
      &parent, QStringList{"invalid"}));

  QVERIFY(!Feature::getOrNull("testFeatureAddon"));
  Feature feature("testFeatureAddon", "Feature Addon",
                  false,               // Is Major Feature
                  L18nStrings::Empty,  // Display name
                  L18nStrings::Empty,  // Description
                  L18nStrings::Empty,  // LongDescr
                  "",                  // ImagePath
                  "",                  // IconPath
                  "",                  // link URL
                  "1.0",               // released
                  true,                // Can be flipped on
                  true,                // Can be flipped off
                  QStringList(),       // feature dependencies
                  []() -> bool { return false; });
  QVERIFY(!!Feature::get("testFeatureAddon"));
  QVERIFY(!Feature::get("testFeatureAddon")->isSupported());

  // A condition not enabled by default
  AddonConditionWatcher* acw =
      AddonConditionWatcherFeaturesEnabled::maybeCreate(
          &parent, QStringList{"testFeatureAddon"});
  QVERIFY(!!acw);
  QVERIFY(!acw->conditionApplied());

  QSignalSpy signalSpy(acw, &AddonConditionWatcher::conditionChanged);
  QCOMPARE(signalSpy.count(), 0);

  FeatureModel* fm = FeatureModel::instance();

  fm->toggle("testFeatureAddon");
  QVERIFY(Feature::get("testFeatureAddon")->isSupported());
  QCOMPARE(signalSpy.count(), 1);
  QVERIFY(acw->conditionApplied());

  // A condition enabled by default
  {
    AddonConditionWatcher* acw2 =
        AddonConditionWatcherFeaturesEnabled::maybeCreate(
            &parent, QStringList{"testFeatureAddon"});
    QVERIFY(!!acw2);
    QVERIFY(acw2->conditionApplied());
  }

  fm->toggle("testFeatureAddon");
  QVERIFY(!Feature::get("testFeatureAddon")->isSupported());
  QCOMPARE(signalSpy.count(), 2);
  QVERIFY(!acw->conditionApplied());
}

void TestAddon::conditionWatcher_group() {
  SettingsHolder settingsHolder;

  QObject parent;
  AddonConditionWatcher* acw1 =
      AddonConditionWatcherTriggerTimeSecs::maybeCreate(&parent, 1);
  QVERIFY(!!acw1);
  QVERIFY(!acw1->conditionApplied());

  AddonConditionWatcher* acw2 =
      AddonConditionWatcherTriggerTimeSecs::maybeCreate(&parent, 2);
  QVERIFY(!!acw2);
  QVERIFY(!acw2->conditionApplied());

  AddonConditionWatcher* acwGroup = new AddonConditionWatcherGroup(
      &parent, QList<AddonConditionWatcher*>{acw1, acw2});
  QVERIFY(!acwGroup->conditionApplied());

  QEventLoop loop;
  bool currentStatus = false;
  connect(acw1, &AddonConditionWatcher::conditionChanged, [&](bool status) {
    currentStatus = status;
    loop.exit();
  });
  loop.exec();

  QVERIFY(currentStatus);
  QVERIFY(acw1->conditionApplied());
  QVERIFY(!acw2->conditionApplied());
  QVERIFY(!acwGroup->conditionApplied());

  currentStatus = false;
  connect(acw2, &AddonConditionWatcher::conditionChanged, [&](bool status) {
    currentStatus = status;
    loop.exit();
  });
  loop.exec();

  QVERIFY(currentStatus);
  QVERIFY(acw1->conditionApplied());
  QVERIFY(acw2->conditionApplied());
  QVERIFY(acwGroup->conditionApplied());
}

void TestAddon::conditionWatcher_triggerTime() {
  SettingsHolder settingsHolder;

  QObject parent;
  AddonConditionWatcher* acw =
      AddonConditionWatcherTriggerTimeSecs::maybeCreate(&parent, 1);
  QVERIFY(!!acw);

  QVERIFY(!acw->conditionApplied());

  QEventLoop loop;
  bool currentStatus = false;
  connect(acw, &AddonConditionWatcher::conditionChanged, [&](bool status) {
    currentStatus = status;
    loop.exit();
  });
  loop.exec();

  QVERIFY(currentStatus);
  QVERIFY(acw->conditionApplied());
}

void TestAddon::conditionWatcher_startTime() {
  SettingsHolder settingsHolder;

  QObject parent;
  AddonConditionWatcher* acw = new AddonConditionWatcherTimeStart(&parent, 0);
  QVERIFY(acw->conditionApplied());

  acw = new AddonConditionWatcherTimeStart(
      &parent, QDateTime::currentSecsSinceEpoch() + 1);
  QVERIFY(!acw->conditionApplied());

  QEventLoop loop;
  bool currentStatus = false;
  connect(acw, &AddonConditionWatcher::conditionChanged, [&](bool status) {
    currentStatus = status;
    loop.exit();
  });
  loop.exec();

  QVERIFY(currentStatus);
  QVERIFY(acw->conditionApplied());
}

void TestAddon::conditionWatcher_endTime() {
  SettingsHolder settingsHolder;

  QObject parent;
  AddonConditionWatcher* acw = new AddonConditionWatcherTimeEnd(&parent, 0);
  QVERIFY(!acw->conditionApplied());

  acw = new AddonConditionWatcherTimeEnd(
      &parent, QDateTime::currentSecsSinceEpoch() + 1);
  QVERIFY(acw->conditionApplied());

  QEventLoop loop;
  bool currentStatus = false;
  connect(acw, &AddonConditionWatcher::conditionChanged, [&](bool status) {
    currentStatus = status;
    loop.exit();
  });
  loop.exec();

  QVERIFY(!currentStatus);
  QVERIFY(!acw->conditionApplied());
}

void TestAddon::guide_create_data() {
  QTest::addColumn<QString>("id");
  QTest::addColumn<QJsonObject>("content");
  QTest::addColumn<bool>("created");

  QTest::addRow("object-without-id") << "" << QJsonObject() << false;

  QJsonObject obj;
  obj["id"] = "foo";
  QTest::addRow("no-image") << "foo" << obj << false;

  obj["image"] = "foo.png";
  QTest::addRow("no-blocks") << "foo" << obj << false;

  QJsonArray blocks;
  obj["blocks"] = blocks;
  QTest::addRow("with-blocks") << "foo" << obj << true;

  blocks.append("");
  obj["blocks"] = blocks;
  QTest::addRow("with-invalid-block") << "foo" << obj << false;

  QJsonObject block;
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-without-id") << "foo" << obj << false;

  block["id"] = "A";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-without-type") << "foo" << obj << false;

  block["type"] = "wow";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-with-invalid-type") << "foo" << obj << false;

  block["type"] = "title";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-title") << "foo" << obj << true;

  block["type"] = "text";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-text") << "foo" << obj << true;

  block["type"] = "olist";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-without-content")
      << "foo" << obj << false;

  block["content"] = "foo";
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-with-invalid-content")
      << "foo" << obj << false;

  QJsonArray content;
  block["content"] = content;
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-with-empty-content")
      << "foo" << obj << true;

  content.append("foo");
  block["content"] = content;
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-with-invalid-content")
      << "foo" << obj << false;

  QJsonObject subBlock;
  content.replace(0, subBlock);
  block["content"] = content;
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-without-id-subblock")
      << "foo" << obj << false;

  subBlock["id"] = "sub";
  content.replace(0, subBlock);
  block["content"] = content;
  blocks.replace(0, block);
  obj["blocks"] = blocks;
  QTest::addRow("with-block-type-olist-with-subblock") << "foo" << obj << true;

  obj["advanced"] = true;
  QTest::addRow("advanced") << "foo" << obj << true;

  obj["advanced"] = false;
  QTest::addRow("not-advanced") << "foo" << obj << true;
}

void TestAddon::guide_create() {
  QFETCH(QString, id);
  QFETCH(QJsonObject, content);
  QFETCH(bool, created);

  SettingsHolder settingsHolder;

  QJsonObject obj;
  obj["guide"] = content;

  QObject parent;
  Addon* guide = AddonGuide::create(&parent, "foo", "bar", "name", obj);
  QCOMPARE(!!guide, created);

  if (!guide) {
    return;
  }

  QCOMPARE(guide->property("title").type(), QMetaType::QString);
  QCOMPARE(guide->property("subtitle").type(), QMetaType::QString);

  QCOMPARE(guide->property("image").toString(), "foo.png");
  QCOMPARE(guide->property("advanced").toBool(), content["advanced"].toBool());
}

void TestAddon::tutorial_create_data() {
  QTest::addColumn<QString>("id");
  QTest::addColumn<QJsonObject>("content");
  QTest::addColumn<bool>("created");

  QTest::addRow("object-without-id") << "" << QJsonObject() << false;

  QJsonObject obj;
  obj["id"] = "foo";
  QTest::addRow("invalid-id") << "foo" << obj << false;
  QTest::addRow("no-image") << "foo" << obj << false;

  obj["image"] = "foo.png";
  QTest::addRow("no-steps") << "foo" << obj << false;

  QJsonArray steps;
  obj["steps"] = steps;
  QTest::addRow("with-steps") << "foo" << obj << false;

  steps.append("");
  obj["steps"] = steps;
  QTest::addRow("with-invalid-step") << "foo" << obj << false;

  QJsonObject step;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-without-id") << "foo" << obj << false;

  step["id"] = "s1";
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-without-element") << "foo" << obj << false;

  step["element"] = "wow";
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-without-next") << "foo" << obj << false;

  step["next"] = "wow";
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next") << "foo" << obj << false;

  QJsonObject nextObj;

  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-1") << "foo" << obj << false;

  nextObj["op"] = "wow";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-2") << "foo" << obj << false;

  nextObj["op"] = "signal";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-3") << "foo" << obj << false;

  nextObj["signal"] = "a";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-4") << "foo" << obj << false;

  nextObj["qml_emitter"] = "a";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-5") << "foo" << obj << true;

  nextObj["vpn_emitter"] = "a";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-6") << "foo" << obj << false;

  nextObj.remove("qml_emitter");
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-7") << "foo" << obj << false;

  nextObj["vpn_emitter"] = "settingsHolder";
  step["next"] = nextObj;
  steps.replace(0, step);
  obj["steps"] = steps;
  QTest::addRow("with-step-with-invalid-next-8") << "foo" << obj << true;

  obj["conditions"] = QJsonObject();
  QTest::addRow("with-step-element and conditions") << "foo" << obj << true;

  obj["advanced"] = true;
  QTest::addRow("advanced") << "foo" << obj << true;

  obj["advanced"] = false;
  QTest::addRow("not-advanced") << "foo" << obj << true;

  obj["advanced"] = true;
  obj["highlighted"] = true;
  QTest::addRow("advanced-and-highlighted") << "foo" << obj << true;
}

void TestAddon::tutorial_create() {
  QFETCH(QString, id);
  QFETCH(QJsonObject, content);
  QFETCH(bool, created);

  SettingsHolder settingsHolder;

  QJsonObject obj;
  obj["tutorial"] = content;

  QObject parent;
  Addon* tutorial = AddonTutorial::create(&parent, "foo", "bar", "name", obj);
  QCOMPARE(!!tutorial, created);

  if (!tutorial) {
    return;
  }

  Tutorial* tm = Tutorial::instance();
  QVERIFY(!!tm);
  QVERIFY(!tm->isPlaying());

  QCOMPARE(tutorial->property("title").type(), QMetaType::QString);
  QCOMPARE(tutorial->property("subtitle").type(), QMetaType::QString);
  QCOMPARE(tutorial->property("completionMessage").type(), QMetaType::QString);
  QCOMPARE(tutorial->property("image").toString(), "foo.png");

  bool isAdvanced =
      content["highlighted"].toBool() ? false : content["advanced"].toBool();
  QCOMPARE(tutorial->property("advanced").toBool(), isAdvanced);

  QQmlApplicationEngine engine;
  QmlEngineHolder qml(&engine);

  QSignalSpy signalSpy(tm, &Tutorial::playingChanged);

  tm->play(tutorial);
  QCOMPARE(signalSpy.count(), 1);

  tm->stop();
  QCOMPARE(signalSpy.count(), 2);
}

void TestAddon::message_create_data() {
  QTest::addColumn<QString>("id");
  QTest::addColumn<QJsonObject>("content");
  QTest::addColumn<bool>("created");

  QTest::addRow("object-without-id") << "" << QJsonObject() << false;

  QJsonObject obj;
  obj["id"] = "foo";
  QTest::addRow("no blocks") << "foo" << obj << false;

  obj["blocks"] = QJsonArray();
  QTest::addRow("good but empty") << "foo" << obj << true;
}

void TestAddon::message_create() {
  QFETCH(QString, id);
  QFETCH(QJsonObject, content);
  QFETCH(bool, created);

  SettingsHolder settingsHolder;

  QJsonObject obj;
  obj["message"] = content;

  QObject parent;
  Addon* message = AddonMessage::create(&parent, "foo", "bar", "name", obj);
  QCOMPARE(!!message, created);

  if (!message) {
    return;
  }

  QCOMPARE(message->property("title").type(), QMetaType::QString);
}

void TestAddon::message_date_data() {
  QTest::addColumn<QString>("languageCode");
  QTest::addColumn<QDateTime>("now");
  QTest::addColumn<QDateTime>("date");
  QTest::addColumn<QString>("result");
  QTest::addColumn<qint64>("timer");

  QTest::addRow("en - future")
      << "en" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(11, 0), QTimeZone(0)) << "10:00 AM"
      << (qint64)(14 * 3600);
  QTest::addRow("it - future")
      << "it" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(11, 0), QTimeZone(0)) << "10:00"
      << (qint64)(14 * 3600);

  QTest::addRow("en - same")
      << "en" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0)) << "10:00 AM"
      << (qint64)(14 * 3600);
  QTest::addRow("it - same")
      << "it" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0)) << "10:00"
      << (qint64)(14 * 3600);

  QTest::addRow("en - one hour ago")
      << "en" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(9, 0), QTimeZone(0)) << "9:00 AM"
      << (qint64)(15 * 3600);
  QTest::addRow("it - one hour ago")
      << "it" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(9, 0), QTimeZone(0)) << "09:00"
      << (qint64)(15 * 3600);

  QTest::addRow("en - midnight")
      << "en" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(0, 0), QTimeZone(0)) << "12:00 AM"
      << (qint64)(24 * 3600);
  QTest::addRow("it - midnight")
      << "it" << QDateTime(QDate(2000, 1, 1), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(0, 0), QTimeZone(0)) << "00:00"
      << (qint64)(24 * 3600);

  QTest::addRow("en - yesterday but less than 24 hours")
      << "en" << QDateTime(QDate(2000, 1, 2), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(21, 0), QTimeZone(0)) << "Yesterday"
      << (qint64)(3 * 3600);

  QTest::addRow("en - yesterday more than 24 hours")
      << "en" << QDateTime(QDate(2000, 1, 2), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 1), QTime(9, 0), QTimeZone(0)) << "Yesterday"
      << (qint64)-1;

  QTest::addRow("en - 2 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 8), QTime(10, 0), QTimeZone(0)) << "Saturday"
      << (qint64)-1;

  QTest::addRow("en - 3 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 7), QTime(10, 0), QTimeZone(0)) << "Friday"
      << (qint64)-1;

  QTest::addRow("en - 4 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 6), QTime(10, 0), QTimeZone(0)) << "Thursday"
      << (qint64)-1;

  QTest::addRow("en - 5 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 5), QTime(10, 0), QTimeZone(0)) << "Wednesday"
      << (qint64)-1;

  QTest::addRow("en - 6 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 4), QTime(10, 0), QTimeZone(0)) << "Tuesday"
      << (qint64)-1;

  QTest::addRow("en - 7 days ago")
      << "en" << QDateTime(QDate(2000, 1, 10), QTime(10, 0), QTimeZone(0))
      << QDateTime(QDate(2000, 1, 3), QTime(10, 0), QTimeZone(0)) << "1/3/00"
      << (qint64)-1;
}

void TestAddon::message_date() {
  SettingsHolder settingsHolder;
  Localizer localizer;

  QFETCH(QString, languageCode);
  localizer.setCode(languageCode);

  QFETCH(QDateTime, now);
  QVERIFY(now.isValid());

  QFETCH(QDateTime, date);
  QVERIFY(date.isValid());

  QFETCH(QString, result);
  QCOMPARE(AddonMessage::dateInternal(now, date), result);

  QFETCH(qint64, timer);
  QCOMPARE(AddonMessage::planDateRetranslationInternal(now, date), timer);
}

void TestAddon::message_dismiss() {
  SettingsHolder settingsHolder;

  QJsonObject messageObj;
  messageObj["id"] = "foo";
  messageObj["blocks"] = QJsonArray();

  QJsonObject obj;
  obj["message"] = messageObj;

  QObject parent;
  Addon* message = AddonMessage::create(&parent, "foo", "bar", "name", obj);
  QVERIFY(!!message);
  QVERIFY(message->enabled());

  // After dismissing the message, it becomes inactive.
  static_cast<AddonMessage*>(message)->dismiss();
  QVERIFY(!message->enabled());

  // No new messages are loaded for the same ID:
  Addon* message2 = AddonMessage::create(&parent, "foo", "bar", "name", obj);
  QVERIFY(!message2);
}

static TestAddon s_testAddon;
