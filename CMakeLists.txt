cmake_minimum_required(VERSION 3.1)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME "evernus")
set(MAJOR_VERSION "1")
set(MINOR_VERSION "51")

find_package(Boost REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5Widgets REQUIRED)

if(${Qt5XmlPatterns_VERSION} VERSION_LESS "5.4.0")
    message(SEND_ERROR "Qt version >= 5.4 is required.")
endif()

if(DEFINED EVERNUS_CLIENT_ID AND DEFINED EVERNUS_CLIENT_SECRET)
    add_definitions(-DEVERNUS_CLIENT_ID=${EVERNUS_CLIENT_ID} -DEVERNUS_CLIENT_SECRET=${EVERNUS_CLIENT_SECRET})
endif()

configure_file(Version.h.in Version.h)

if(APPLE)
    find_package(Qt5MacExtras REQUIRED)
    find_library(CarbonLibrary Carbon)
    mark_as_advanced(CarbonLibrary)

    configure_file("${CMAKE_SOURCE_DIR}/Info.plist.in" Info.plist)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.8")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -pthread")

    # From Qt 5.6 onwards SSL parts use native MacOS backend
    # https://forum.qt.io/topic/55853/openssl-and-mac-os-x/6
    # but LibQxt checks for QT_NO_OPENSSL and disables ssl in this case.
    # To workaround that we define QT_OPENSSL - see qconfig.h
    add_definitions(-DQT_OPENSSL)

    set(ADDITIONAL_SRC qxtglobalshortcut_mac.cpp)
    set(ADDITIONAL_LIBS Qt5::MacExtras ${CarbonLibrary})
elseif(UNIX)
    find_package(X11 REQUIRED)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g")
    set(ADDITIONAL_SRC qxtglobalshortcut_x11.cpp)
    set(ADDITIONAL_LIBS ${X11_LIBRARIES})
elseif(WIN32)
    find_package(Qt5WinExtras REQUIRED)

    set(ADDITIONAL_SRC evernus.rc qxtglobalshortcut_win.cpp)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP /wd4503")
    set(ADDITIONAL_LIBS Qt5::WinExtras)

    if(MSVC_VERSION LESS 1900) # < VS2015
        message(FATAL_ERROR "Compiler too old, VS 2015 or newer required.")
    endif() # > VS2015
endif()

set(TRANSLATIONS lang/lang_pl_PL.ts lang/lang_de_DE.ts)
set_source_files_properties(${TRANSLATIONS} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/trans")

add_definitions(-DQXT_STATIC)

if(DEFINED EVERNUS_DROPBOX_APP_KEY AND DEFINED EVERNUS_DROPBOX_APP_SECRET)
    add_definitions(-DEVERNUS_DROPBOX_APP_KEY=${EVERNUS_DROPBOX_APP_KEY} -DEVERNUS_DROPBOX_APP_SECRET=${EVERNUS_DROPBOX_APP_SECRET})

    if(DEFINED QTDROPBOX_DEBUG)
        add_definitions(-DQTDROPBOX_DEBUG)
    endif()
endif()

if(DEFINED EVERNUS_ESI_SISI)
    add_definitions(-DEVERNUS_ESI_SISI)
endif()

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TRANSLATIONS})

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

add_executable(
    ${PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    AboutDialog.cpp
    AboutDialog.h
    ActiveTasksDialog.cpp
    ActiveTasksDialog.h
    AdjustableTableView.cpp
    AdjustableTableView.h
    AggregatedStatisticsModel.cpp
    AggregatedStatisticsModel.h
    APIDomParser.h
    APIInterface.cpp
    APIInterface.h
    APIManager.cpp
    APIManager.h
    APIResponseCache.cpp
    APIResponseCache.h
    APIUtils.cpp
    APIUtils.h
    APIXmlReceiver.h
    AssetList.cpp
    AssetList.h
    AssetListRepository.cpp
    AssetListRepository.h
    AssetListXmlReceiver.cpp
    AssetListXmlReceiver.h
    AssetModel.cpp
    AssetModel.h
    AssetProvider.h
    AssetsImportPreferencesWidget.cpp
    AssetsImportPreferencesWidget.h
    AssetsWidget.cpp
    AssetsWidget.h
    AssetValueSnapshot.cpp
    AssetValueSnapshot.h
    AssetValueSnapshotRepository.cpp
    AssetValueSnapshotRepository.h
    AssignedContractModel.cpp
    AssignedContractModel.h
    ButtonWithTimer.cpp
    ButtonWithTimer.h
    CacheTimer.cpp
    CacheTimer.h
    CacheTimerProvider.h
    CacheTimerRepository.cpp
    CacheTimerRepository.h
    CachingAssetProvider.cpp
    CachingAssetProvider.h
    CachingContractProvider.cpp
    CachingContractProvider.h
    CachingEveDataProvider.cpp
    CachingEveDataProvider.h
    CachingMarketOrderProvider.cpp
    CachingMarketOrderProvider.h
    CallbackExternalOrderImporter.cpp
    CallbackExternalOrderImporter.h
    Character.cpp
    Character.h
    CharacterBoundWidget.cpp
    CharacterBoundWidget.h
    CharacterCorporationCombinedMarketOrderProvider.cpp
    CharacterCorporationCombinedMarketOrderProvider.h
    CharacterData.h
    CharacterDomParser.cpp
    CharacterDomParser.h
    CharacterImportPreferencesWidget.cpp
    CharacterImportPreferencesWidget.h
    CharacterListXmlReceiver.cpp
    CharacterListXmlReceiver.h
    CharacterManagerDialog.cpp
    CharacterManagerDialog.h
    CharacterModel.cpp
    CharacterModel.h
    CharacterRepository.cpp
    CharacterRepository.h
    CharacterWidget.cpp
    CharacterWidget.h
    Citadel.cpp
    Citadel.h
    CitadelManager.cpp
    CitadelManager.h
    CitadelRepository.cpp
    CitadelRepository.h
    ClickableLabel.cpp
    ClickableLabel.h
    ColorButton.cpp
    ColorButton.h
    CommonScriptAPI.cpp
    CommonScriptAPI.h
    ConquerableStation.cpp
    ConquerableStation.h
    ConquerableStationList.h
    ConquerableStationListXmlReceiver.cpp
    ConquerableStationListXmlReceiver.h
    ConquerableStationRepository.cpp
    ConquerableStationRepository.h
    Contract.cpp
    Contract.h
    ContractFilterProxyModel.cpp
    ContractFilterProxyModel.h
    ContractImportPreferencesWidget.cpp
    ContractImportPreferencesWidget.h
    ContractItem.cpp
    ContractItem.h
    ContractItemRepository.cpp
    ContractItemRepository.h
    ContractItemsXmlReceiver.cpp
    ContractItemsXmlReceiver.h
    ContractModel.cpp
    ContractModel.h
    ContractProvider.h
    ContractRepository.cpp
    ContractRepository.h
    Contracts.h
    ContractStatusesWidget.cpp
    ContractStatusesWidget.h
    ContractsXmlReceiver.cpp
    ContractsXmlReceiver.h
    ContractView.cpp
    ContractView.h
    ContractWidget.cpp
    ContractWidget.h
    CorpAssetValueSnapshot.cpp
    CorpAssetValueSnapshot.h
    CorpAssetValueSnapshotRepository.cpp
    CorpAssetValueSnapshotRepository.h
    CorpImportPreferencesWidget.cpp
    CorpImportPreferencesWidget.h
    CorpKey.cpp
    CorpKey.h
    CorpKeyEditDialog.cpp
    CorpKeyEditDialog.h
    CorpKeyRepository.cpp
    CorpKeyRepository.h
    CorpMarketOrderValueSnapshot.cpp
    CorpMarketOrderValueSnapshot.h
    CorpMarketOrderValueSnapshotRepository.cpp
    CorpMarketOrderValueSnapshotRepository.h
    CorpWalletSnapshot.cpp
    CorpWalletSnapshot.h
    CorpWalletSnapshotRepository.cpp
    CorpWalletSnapshotRepository.h
    CustomFPCDialog.cpp
    CustomFPCDialog.h
    DatabaseUtils.cpp
    DatabaseUtils.h
    DateFilteredPlotWidget.cpp
    DateFilteredPlotWidget.h
    DateRangeWidget.cpp
    DateRangeWidget.h
    Defines.h
    DeviationSourceWidget.cpp
    DeviationSourceWidget.h
    Entity.h
    ESIIndividualExternalOrderImporter.cpp
    ESIIndividualExternalOrderImporter.h
    ESIInterface.cpp
    ESIInterface.h
    ESIManager.h
    ESIManager.cpp
    ESIWholeExternalOrderImporter.cpp
    ESIWholeExternalOrderImporter.h
    EveCentralExternalOrderImporter.cpp
    EveCentralExternalOrderImporter.h
    EveCentralManager.cpp
    EveCentralManager.h
    EveDataProvider.h
    evernus.qrc
    EvernusApplication.cpp
    EvernusApplication.h
    EveType.cpp
    EveType.h
    EveTypeRepository.cpp
    EveTypeRepository.h
    ExternalOrder.cpp
    ExternalOrder.h
    ExternalOrderBuyModel.cpp
    ExternalOrderBuyModel.h
    ExternalOrderFilterProxyModel.cpp
    ExternalOrderFilterProxyModel.h
    ExternalOrderImporter.h
    ExternalOrderImporterNames.h
    ExternalOrderImporterRegistry.h
    ExternalOrderModel.h
    ExternalOrderRepository.cpp
    ExternalOrderRepository.h
    ExternalOrderSellModel.cpp
    ExternalOrderSellModel.h
    ExternalOrderView.cpp
    ExternalOrderView.h
    FavoriteItem.h
    FavoriteItemRepository.cpp
    FavoriteItemRepository.h
    FileDownload.cpp
    FileDownload.h
    FilterText.cpp
    FilterText.h
    FilterTextRepository.cpp
    FilterTextRepository.h
    FlowLayout.cpp
    FlowLayout.h
    FPCController.cpp
    FPCController.h
    GeneralPreferencesWidget.cpp
    GeneralPreferencesWidget.h
    GenericMarketOrdersInfoWidget.cpp
    GenericMarketOrdersInfoWidget.h
    GenericNameDomParser.cpp
    GenericNameDomParser.h
    HttpPreferencesWidget.cpp
    HttpPreferencesWidget.h
    HttpService.cpp
    HttpService.h
    HttpSettings.h
    IconUtils.cpp
    IconUtils.h
    ImportPreferencesWidget.cpp
    ImportPreferencesWidget.h
    ImportSettings.h
    ImportSourcePreferencesWidget.cpp
    ImportSourcePreferencesWidget.h
    InterRegionMarketDataFilterProxyModel.cpp
    InterRegionMarketDataFilterProxyModel.h
    InterRegionMarketDataModel.cpp
    InterRegionMarketDataModel.h
    IssuedContractModel.cpp
    IssuedContractModel.h
    Item.cpp
    Item.h
    ItemCost.cpp
    ItemCost.h
    ItemCostEditDialog.cpp
    ItemCostEditDialog.h
    ItemCostModel.cpp
    ItemCostModel.h
    ItemCostProvider.h
    ItemCostRepository.cpp
    ItemCostRepository.h
    ItemCostWidget.cpp
    ItemCostWidget.h
    ItemData.h
    ItemHistoriesWidget.cpp
    ItemHistoriesWidget.h
    ItemHistoryWidget.cpp
    ItemHistoryWidget.h
    ItemNameModel.cpp
    ItemNameModel.h
    ItemRepository.cpp
    ItemRepository.h
    ItemTypeSelectDialog.cpp
    ItemTypeSelectDialog.h
    JSEveDataProvider.cpp
    JSEveDataProvider.h
    Key.cpp
    Key.h
    KeyEditDialog.cpp
    KeyEditDialog.h
    KeyRepository.cpp
    KeyRepository.h
    LanguageComboBox.cpp
    LanguageComboBox.h
    LanguageSelectDialog.cpp
    LanguageSelectDialog.h
    LeafFilterProxyModel.cpp
    LeafFilterProxyModel.h
    LMeveAPIManager.cpp
    LMeveAPIManager.h
    LMeveAPIInterface.cpp
    LMeveAPIInterface.h
    LMeveDataProvider.h
    LMevePreferencesWidget.cpp
    LMevePreferencesWidget.h
    LMeveTaskModel.cpp
    LMeveTaskModel.h
    LMeveTaskRepository.cpp
    LMeveTaskRepository.h
    LMeveWidget.cpp
    LMeveWidget.h
    LMeveSettings.h
    LMeveTask.cpp
    LMeveTask.h
    LocationBookmark.cpp
    LocationBookmark.h
    LocationBookmarkRepository.cpp
    LocationBookmarkRepository.h
    LocationBookmarkSelectDialog.cpp
    LocationBookmarkSelectDialog.h
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MarginToolDialog.cpp
    MarginToolDialog.h
    MarginToolSettings.h
    MarketAnalysisDataFetcher.cpp
    MarketAnalysisDataFetcher.h
    MarketAnalysisSettings.h
    MarketAnalysisWidget.h
    MarketAnalysisWidget.cpp
    MarketBrowserWidget.cpp
    MarketBrowserWidget.h
    MarketGroup.cpp
    MarketGroup.h
    MarketGroupRepository.cpp
    MarketGroupRepository.h
    MarketHistoryEntry.h
    MarketLogExternalOrderImporter.cpp
    MarketLogExternalOrderImporter.h
    MarketLogExternalOrderImporterThread.cpp
    MarketLogExternalOrderImporterThread.h
    MarketOrder.cpp
    MarketOrder.h
    MarketOrderArchiveModel.cpp
    MarketOrderArchiveModel.h
    MarketOrderBuyModel.cpp
    MarketOrderBuyModel.h
    MarketOrderFilterProxyModel.cpp
    MarketOrderFilterProxyModel.h
    MarketOrderFilterWidget.cpp
    MarketOrderFilterWidget.h
    MarketOrderInfoWidget.cpp
    MarketOrderInfoWidget.h
    MarketOrderModel.h
    MarketOrderPriceStatusesWidget.cpp
    MarketOrderPriceStatusesWidget.h
    MarketOrderProvider.h
    MarketOrderRepository.cpp
    MarketOrderRepository.h
    MarketOrders.h
    MarketOrderSellModel.cpp
    MarketOrderSellModel.h
    MarketOrdersInfoWidget.h
    MarketOrderStatesWidget.cpp
    MarketOrderStatesWidget.h
    MarketOrdersXmlReceiver.cpp
    MarketOrdersXmlReceiver.h
    MarketOrderTreeModel.cpp
    MarketOrderTreeModel.h
    MarketOrderValueSnapshot.cpp
    MarketOrderValueSnapshot.h
    MarketOrderValueSnapshotRepository.cpp
    MarketOrderValueSnapshotRepository.h
    MarketOrderView.cpp
    MarketOrderView.h
    MarketOrderViewWithTransactions.cpp
    MarketOrderViewWithTransactions.h
    MarketOrderVolumeItemDelegate.cpp
    MarketOrderVolumeItemDelegate.h
    MarketOrderWidget.cpp
    MarketOrderWidget.h
    MathUtils.h
    MenuBarWidget.cpp
    MenuBarWidget.h
    MetaGroup.cpp
    MetaGroup.h
    MetaGroupRepository.cpp
    MetaGroupRepository.h
    NetworkPreferencesWidget.cpp
    NetworkPreferencesWidget.h
    NetworkSettings.h
    NumberFormatDelegate.cpp
    NumberFormatDelegate.h
    OrderScript.cpp
    OrderScript.h
    OrderScriptRepository.cpp
    OrderScriptRepository.h
    OrderSettings.h
    OrderPreferencesWidget.cpp
    OrderPreferencesWidget.h
    PathPreferencesWidget.cpp
    PathPreferencesWidget.h
    PathSettings.h
    PathUtils.cpp
    PathUtils.h
    PreferencesDialog.cpp
    PreferencesDialog.h
    PricePreferencesWidget.cpp
    PricePreferencesWidget.h
    PriceSettings.h
    PriceType.h
    PriceUtils.cpp
    PriceUtils.h
    ProgressiveCounter.cpp
    ProgressiveCounter.h
    ProxyWebExternalOrderImporter.cpp
    ProxyWebExternalOrderImporter.h
    qcustomplot.cpp
    qcustomplot.h
    qdropbox2.cpp
    qdropbox2.h
    qdropbox2account.cpp
    qdropbox2account.h
    qdropbox2common.h
    qdropbox2entity.h
    qdropbox2entityinfo.cpp
    qdropbox2entityinfo.h
    qdropbox2file.cpp
    qdropbox2file.h
    qdropbox2folder.cpp
    qdropbox2folder.h
    qdropbox2global.h
    qhttpheader.cpp
    qhttpheader.h
    QtScriptSyntaxHighlighter.cpp
    QtScriptSyntaxHighlighter.h
    qxtabstracthttpconnector.cpp
    qxtabstracthttpconnector.h
    qxtabstractwebservice.cpp
    qxtabstractwebservice.h
    qxtabstractwebsessionmanager_p.h
    qxtabstractwebsessionmanager.cpp
    qxtabstractwebsessionmanager.h
    qxtboundcfunction.h
    qxtboundfunction.h
    qxtboundfunctionbase.h
    qxtfifo.cpp
    qxtfifo.h
    qxtglobal.cpp
    qxtglobal.h
    qxtglobalshortcut.cpp
    qxtglobalshortcut.h
    qxtglobalshortcut_p.h
    qxthmac.cpp
    qxthmac.h
    qxthtmltemplate.cpp
    qxthtmltemplate.h
    qxthttpserverconnector.cpp
    qxthttpsessionmanager.cpp
    qxthttpsessionmanager.h
    qxtmail_p.h
    qxtmailattachment.cpp
    qxtmailattachment.h
    qxtmailmessage.cpp
    qxtmailmessage.h
    qxtmetaobject.cpp
    qxtmetaobject.h
    qxtmetatype.h
    qxtnull.cpp
    qxtnull.h
    qxtnullable.h
    qxtscgiserverconnector.cpp
    qxtsmtp_p.h
    qxtsmtp.cpp
    qxtsmtp.h
    qxtsslserver.cpp
    qxtsslserver.h
    qxtwebcontent.cpp
    qxtwebcontent.h
    qxtwebevent.cpp
    qxtwebevent.h
    qxtwebslotservice.cpp
    qxtwebslotservice.h
    RefType.cpp
    RefType.h
    RefTypeRepository.cpp
    RefTypeRepository.h
    RefTypeXmlReceiver.cpp
    RefTypeXmlReceiver.h
    RegionTypeSelectDialog.cpp
    RegionTypeSelectDialog.h
    ReplyTimeout.cpp
    ReplyTimeout.h
    Repository.h
    RepositoryProvider.h
    ScriptOrderProcessingModel.cpp
    ScriptOrderProcessingModel.h
    ScriptUtils.cpp
    ScriptUtils.h
    SecurityHelper.cpp
    SecurityHelper.h
    SellMarketOrdersInfoWidget.cpp
    SellMarketOrdersInfoWidget.h
    SimpleCrypt.cpp
    SimpleCrypt.h
    SoundPreferencesWidget.cpp
    SoundPreferencesWidget.h
    SoundSettings.h
    SSOAuthWidget.cpp
    SSOAuthWidget.h
    SSOMessageBox.cpp
    SSOMessageBox.h
    SSOSettings.h
    SSOUtils.cpp
    SSOUtils.h
    StationModel.cpp
    StationModel.h
    StationSelectDialog.cpp
    StationSelectDialog.h
    StationView.cpp
    StationView.h
    StatisticsPreferencesWidget.cpp
    StatisticsPreferencesWidget.h
    StatisticsSettings.h
    StatisticsWidget.cpp
    StatisticsWidget.h
    StructureHuntInterface.cpp
    StructureHuntInterface.h
    StyledTreeView.cpp
    StyledTreeView.h
    StyledTreeViewItemDelegate.cpp
    StyledTreeViewItemDelegate.h
    SyncDialog.cpp
    SyncDialog.h
    SyncPreferencesWidget.cpp
    SyncPreferencesWidget.h
    SyncSettings.h
    TaskConstants.h
    TaskManager.h
    TextFilterWidget.cpp
    TextFilterWidget.h
    TextUtils.cpp
    TextUtils.h
    TimerTypes.h
    TradeableTypesTreeModel.cpp
    TradeableTypesTreeModel.h
    TypeAggregatedDetailsWidget.cpp
    TypeAggregatedDetailsWidget.h
    TypeAggregatedMarketDataFilterProxyModel.cpp
    TypeAggregatedMarketDataFilterProxyModel.h
    TypeAggregatedMarketDataModel.cpp
    TypeAggregatedMarketDataModel.h
    UISettings.h
    Updater.cpp
    Updater.h
    UpdaterSettings.h
    UpdateTimer.cpp
    UpdateTimer.h
    UpdateTimerRepository.cpp
    UpdateTimerRepository.h
    Version.h
    WalletEntryFilterWidget.cpp
    WalletEntryFilterWidget.h
    WalletJournal.h
    WalletJournalEntry.cpp
    WalletJournalEntry.h
    WalletJournalEntryRepository.cpp
    WalletJournalEntryRepository.h
    WalletJournalModel.cpp
    WalletJournalModel.h
    WalletJournalWidget.cpp
    WalletJournalWidget.h
    WalletJournalXmlReceiver.cpp
    WalletJournalXmlReceiver.h
    WalletPreferencesWidget.cpp
    WalletPreferencesWidget.h
    WalletSettings.h
    WalletSnapshot.cpp
    WalletSnapshot.h
    WalletSnapshotRepository.cpp
    WalletSnapshotRepository.h
    WalletTransaction.cpp
    WalletTransaction.h
    WalletTransactionRepository.cpp
    WalletTransactionRepository.h
    WalletTransactions.h
    WalletTransactionsModel.cpp
    WalletTransactionsModel.h
    WalletTransactionsWidget.cpp
    WalletTransactionsWidget.h
    WalletTransactionsXmlReceiver.cpp
    WalletTransactionsXmlReceiver.h
    WalletTransactionView.cpp
    WalletTransactionView.h
    WarningBarWidget.cpp
    WarningBarWidget.h
    ${TRANSLATIONS}
    ${QM_FILES}
    ${ADDITIONAL_SRC}
)

if(WIN32)
    if(${CMAKE_CL_64})
        # cmake bug
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_FLAGS} /machine:x64")
    endif()
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist")

    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_CONFIGURATION_TYPES "Release;MinSizeRel;RelWithDebInfo;Debug")

    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Evernus")

    # Make sure bundle will be always created at the same path
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set(BUNDLE_PATH "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    ${Boost_LIBRARIES}
    Qt5::Core
    Qt5::Gui
    Qt5::Multimedia
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Qml
    Qt5::Sql
    Qt5::Xml
    Qt5::XmlPatterns
    Qt5::WebEngineWidgets
    Qt5::Widgets
    ${ADDITIONAL_LIBS}
)

set(RESOURCES
    "res"
    "${CMAKE_CURRENT_BINARY_DIR}/trans"
)

if(APPLE)
    foreach(RES ${RESOURCES})
        install(DIRECTORY ${RES} DESTINATION "${BUNDLE_PATH}/Contents/MacOS/")
    endforeach()

    install(FILES iconfile.icns DESTINATION "${BUNDLE_PATH}/Contents/Resources/")

    find_path(MACDEPLOYQT_PATH macdeployqt PATH_SUFFIXES bin)
    if(NOT MACDEPLOYQT_PATH)
        message(FATAL_ERROR "Could not find macdeployqt for OSX bundling. You can point MACDEPLOYQT_PATH to it's path.")
    endif()

    # If macdeployqt is found just build and install.
    # With xcode it is:
    # $ xcodebuild -project Evernus.xcodeproj
    # $ xcodebuild -project Evernus.xcodeproj -target install
    # OSX budle is ready.
    install(CODE "
            execute_process(COMMAND ${MACDEPLOYQT_PATH}/macdeployqt \"${BUNDLE_PATH}\" -verbose=2)
            message(STATUS \"Creating dmg of the app\")
            execute_process(COMMAND hdiutil create -volname ${PROJECT_NAME} -srcfolder \"${BUNDLE_PATH}\" -ov -format UDZO \"${PROJECT_NAME}.dmg\")
            " COMPONENT Runtime
    )
elseif(UNIX)
    foreach(RES ${RESOURCES})
        install(DIRECTORY ${RES} DESTINATION "share/Evernus")
    endforeach()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
endif()
