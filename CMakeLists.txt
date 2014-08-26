cmake_minimum_required(VERSION 2.8.11)

project(Evernus)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROJECT_NAME "evernus")

find_package(Boost REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Script REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5XmlPatterns REQUIRED)
find_package(Qt5Widgets REQUIRED)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -stdlib=libc++ -g")
elseif(WIN32)
    find_package(Qt5WinExtras REQUIRED)

    set(ADDITIONAL_SRC "evernus.rc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(ADDITIONAL_LIBS Qt5::WinExtras)
endif()

set(TRANSLATIONS lang/lang_pl_PL.ts)
set_source_files_properties(${TRANSLATIONS} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/trans")

add_definitions("-DQXT_STATIC")

qt5_wrap_cpp(
    MOC_FILES
    AboutDialog.h
    ActiveTasksDialog.h
    AggregatedStatisticsModel.h
    APIInterface.h
    APIManager.h
    APIResponseCache.h
    AssetModel.h
    AssetsImportPreferencesWidget.h
    AssetsWidget.h
    ButtonWithTimer.h
    CacheExternalOrderImporter.h
    CharacterBoundWidget.h
    CharacterManagerDialog.h
    CharacterModel.h
    CharacterImportPreferencesWidget.h
    CharacterWidget.h
    ContractWidget.h
    CorpImportPreferencesWidget.h
    CorpKeyEditDialog.h
    DateFilteredPlotWidget.h
    DateRangeWidget.h
    DeviationSourceWidget.h
    EveMarketDataExternalOrderImporter.h
    EvernusApplication.h
    ExternalOrderBuyModel.h
    ExternalOrderImporter.h
    ExternalOrderSellModel.h
    ExternalOrderView.h
    FileDownload.h
    FilterTextRepository.h
    GeneralPreferencesWidget.h
    HttpPreferencesWidget.h
    HttpService.h
    IGBPreferencesWidget.h
    IGBService.h
    ImportPreferencesWidget.h
    ImportSourcePreferencesWidget.h
    ItemCostEditDialog.h
    ItemCostModel.h
    ItemCostWidget.h
    ItemHistoryWidget.h
    ItemNameModel.h
    ItemTypeSelectDialog.h
    KeyEditDialog.h
    LanguageSelectDialog.h
    LocationBookmarkSelectDialog.h
    MainWindow.h
    MarginToolDialog.h
    MarketBrowserWidget.h
    MarketLogExternalOrderImporter.h
    MarketLogExternalOrderImporterThread.h
    MarketOrderArchiveModel.h
    MarketOrderBuyModel.h
    MarketOrderFilterProxyModel.h
    MarketOrderFilterWidget.h
    MarketOrderInfoWidget.h
    MarketOrderPriceStatusesWidget.h
    MarketOrderSellModel.h
    MarketOrderStatesWidget.h
    MarketOrderView.h
    MarketOrderViewWithTransactions.h
    MarketOrderWidget.h
    MenuBarWidget.h
    NetworkPreferencesWidget.h
    PathPreferencesWidget.h
    PreferencesDialog.h
    PricePreferencesWidget.h
    qcustomplot.h
    qxtabstracthttpconnector.h
    qxtabstractwebservice.h
    qxtabstractwebsessionmanager.h
    qxtabstractwebsessionmanager_p.h
    qxtboundfunction.h
    qxtfifo.h
    qxthttpsessionmanager.h
    qxtsmtp_p.h
    qxtsmtp.h
    qxtsslserver.h
    qxtwebcontent.h
    qxtwebslotservice.h
    ScriptOrderProcessingModel.h
    StationModel.h
    StatisticsWidget.h
    StyledTreeView.h
    TextFilterWidget.h
    Updater.h
    WalletEntryFilterWidget.h
    WalletJournalModel.h
    WalletJournalWidget.h
    WalletPreferencesWidget.h
    WalletTransactionsModel.h
    WalletTransactionsWidget.h
)

qt5_add_resources(
    RCC_FILES
    evernus.qrc
)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TRANSLATIONS})

include_directories(${Boost_INCLUDE_DIRS})

add_executable(
    ${PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    AboutDialog.cpp
    AboutDialog.h
    ActiveTasksDialog.cpp
    ActiveTasksDialog.h
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
    ButtonWithTimer.cpp
    ButtonWithTimer.h
    CacheExternalOrderImporter.cpp
    CacheExternalOrderImporter.h
    CacheTimer.cpp
    CacheTimer.h
    CacheTimerProvider.h
    CacheTimerRepository.cpp
    CacheTimerRepository.h
    CachingMarketOrderProvider.cpp
    CachingMarketOrderProvider.h
    Character.cpp
    Character.h
    CharacterBoundWidget.cpp
    CharacterBoundWidget.h
    CharacterCorporationCombinedMarketOrderProvider.cpp
    CharacterCorporationCombinedMarketOrderProvider.h
    CharacterData.h
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
    ConquerableStation.cpp
    ConquerableStation.h
    ConquerableStationList.h
    ConquerableStationListXmlReceiver.cpp
    ConquerableStationListXmlReceiver.h
    ConquerableStationRepository.cpp
    ConquerableStationRepository.h
    Contract.cpp
    Contract.h
    ContractRepository.cpp
    ContractRepository.h
    Contracts.h
    ContractsXmlReceiver.cpp
    ContractsXmlReceiver.h
    ContractWidget.cpp
    ContractWidget.h
    CorpImportPreferencesWidget.cpp
    CorpImportPreferencesWidget.h
    CorpKey.cpp
    CorpKey.h
    CorpKeyEditDialog.cpp
    CorpKeyEditDialog.h
    CorpKeyRepository.cpp
    CorpKeyRepository.h
    DatabaseUtils.cpp
    DatabaseUtils.h
    DateFilteredPlotWidget.cpp
    DateFilteredPlotWidget.h
    DateRangeWidget.cpp
    DateRangeWidget.h
    DeviationSourceWidget.cpp
    DeviationSourceWidget.h
    Entity.h
    EveCacheBuffer.cpp
    EveCacheBuffer.h
    EveCacheFile.cpp
    EveCacheFile.h
    EveCacheFileParser.cpp
    EveCacheFileParser.h
    EveCacheManager.cpp
    EveCacheManager.h
    EveCacheNodes.cpp
    EveCacheNodes.h
    EveCacheReader.h
    EveDataProvider.h
    EveMarketDataExternalOrderImporter.cpp
    EveMarketDataExternalOrderImporter.h
    EveMarketDataExternalOrderImporterXmlReceiver.cpp
    EveMarketDataExternalOrderImporterXmlReceiver.h
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
    GeneralPreferencesWidget.cpp
    GeneralPreferencesWidget.h
    HttpPreferencesWidget.cpp
    HttpPreferencesWidget.h
    HttpService.cpp
    HttpService.h
    HttpSettings.h
    IconUtils.cpp
    IconUtils.h
    IGBPreferencesWidget.cpp
    IGBPreferencesWidget.h
    IGBService.cpp
    IGBService.h
    IGBSettings.h
    ImportPreferencesWidget.cpp
    ImportPreferencesWidget.h
    ImportSettings.h
    ImportSourcePreferencesWidget.cpp
    ImportSourcePreferencesWidget.h
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
    ItemHistoryWidget.cpp
    ItemHistoryWidget.h
    ItemNameModel.cpp
    ItemNameModel.h
    ItemRepository.cpp
    ItemRepository.h
    ItemTypeSelectDialog.cpp
    ItemTypeSelectDialog.h
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
    MarketBrowserWidget.cpp
    MarketBrowserWidget.h
    MarketGroup.cpp
    MarketGroup.h
    MarketGroupRepository.cpp
    MarketGroupRepository.h
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
    MenuBarWidget.cpp
    MenuBarWidget.h
    MetaGroup.cpp
    MetaGroup.h
    MetaGroupRepository.cpp
    MetaGroupRepository.h
    NetworkPreferencesWidget.cpp
    NetworkPreferencesWidget.h
    NetworkSettings.h
    OrderScript.cpp
    OrderScript.h
    OrderScriptRepository.cpp
    OrderScriptRepository.h
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
    PriceUtils.cpp
    PriceUtils.h
    qcustomplot.cpp
    qcustomplot.h
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
    Repository.h
    ScriptOrderProcessingModel.cpp
    ScriptOrderProcessingModel.h
    ScriptUtils.cpp
    ScriptUtils.h
    SimpleCrypt.cpp
    SimpleCrypt.h
    StationModel.cpp
    StationModel.h
    StatisticsWidget.cpp
    StatisticsWidget.h
    StyledTreeView.cpp
    StyledTreeView.h
    StyledTreeViewItemDelegate.cpp
    StyledTreeViewItemDelegate.h
    TaskConstants.h
    TextFilterWidget.cpp
    TextFilterWidget.h
    TextUtils.cpp
    TextUtils.h
    TimerTypes.h
    UISettings.h
    Updater.cpp
    Updater.h
    UpdaterSettings.h
    UpdateTimer.cpp
    UpdateTimer.h
    UpdateTimerRepository.cpp
    UpdateTimerRepository.h
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
    WarningBarWidget.cpp
    WarningBarWidget.h
    ${MOC_FILES}
    ${RCC_FILES}
    ${TRANSLATIONS}
    ${QM_FILES}
    ${ADDITIONAL_SRC}
)

if(WIN32)
    # cmake bug
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LINK_FLAGS} /machine:x64")
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "Info.plist")

    set(CMAKE_BUILD_TYPE "MinSizeRel")
    set(CMAKE_CONFIGURATION_TYPES "MinSizeRel;Release;RelWithDebInfo;Debug")

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
    Qt5::Network
    Qt5::PrintSupport
    Qt5::Script
    Qt5::Sql
    Qt5::Xml
    Qt5::XmlPatterns
    Qt5::Widgets
    ${ADDITIONAL_LIBS}
)

set(RESOURCES
    "res"
    "trans"
)

if(APPLE)
    foreach(RES ${RESOURCES})
        install(DIRECTORY ${RES} DESTINATION "${BUNDLE_PATH}/Contents/MacOS/")
    endforeach()

    install(FILES iconfile.icns DESTINATION "${BUNDLE_PATH}/Contents/Resources/")

    find_path(MACDEPLOYQT_PATH macdeployqt PATH_SUFFIXES bin)
    if(NOT MACDEPLOYQT_PATH)
        message(FATAL_ERROR "Could not find macdeployqt for OSX bunding. You can point MACDEPLOYQT_PATH to it's path.")
    endif()

    # If macdeployqt is found just build and install.
    # With xcode it is:
    # $ xcodebuild -project Evernus.xcodeproj
    # $ xcodebuild -project Evernus.xcodeproj -target install
    # OSX budle is ready.
    install(CODE "
            execute_process(COMMAND ${MACDEPLOYQT_PATH}/macdeployqt \"${BUNDLE_PATH}\" -verbose=2)
            " COMPONENT Runtime
    )
elseif(UNIX)
    foreach(RES ${RESOURCES})
        install(DIRECTORY ${RES} DESTINATION "/usr/share/Evernus")
    endforeach()
    
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "/usr/bin")
endif()
