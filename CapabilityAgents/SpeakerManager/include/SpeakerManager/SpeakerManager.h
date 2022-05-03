/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGER_H_

#include <future>
#include <map>
#include <memory>
#include <unordered_set>
#include <vector>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace speakerManager {

/**
 * This class implements a @c CapabilityAgent that handles the AVS @c Speaker API.
 *
 * The @c SpeakerManager can handle multiple @c SpeakerInterface objects. @c SpeakerInterface
 * are grouped by their respective types, and the volume and mute state will be consistent
 * across each type. For example, to change the volume of all speakers of a specific type:
 *
 * @code{.cpp}
 *     // Use local setVolume API.
 *     std::future<bool> result = speakerManager->setVolume(type, AVS_SET_VOLUME_MAX);
 *     // Optionally, wait for the operation to complete.
 *     if (result.valid()) {
 *          result.wait();
 *     }
 * @endcode
 *
 * Clients may extend the @c SpeakerInterface::Type enum if multiple independent volume controls are needed.
 */
class SpeakerManager
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::SpeakerManagerInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Create an instance of @c SpeakerManager, and register the @c SpeakerInterfaces that will be controlled
     * by it. SpeakerInterfaces will be grouped by @c SpeakerInterface::Type.
     *
     * @param speakers The @c Speakers to register.
     * @param contextManager A @c ContextManagerInterface to manage the context.
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @param exceptionEncounteredSender An @c ExceptionEncounteredSenderInterface to send
     * directive processing exceptions to AVS.
     */
    static std::shared_ptr<SpeakerManager> create(
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& speakers,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    // @name SpeakerManagerInterface Functions
    /// @{
    std::future<bool> setVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t volume,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;
    std::future<bool> adjustVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t delta,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;
    std::future<bool> setMute(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        bool mute,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API) override;
    std::future<bool> getSpeakerSettings(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings) override;
    void addSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) override;
    void removeSpeakerManagerObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer) override;
    void addSpeaker(std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor. Called after validation has occurred on parameters.
     *
     * @param speakers The @c Speakers to register.
     * @param contextManager A @c ContextManagerInterface to manage the context.
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @param exceptionEncounteredSender An @c ExceptionEncounteredSenderInterface to send.
     * @param minUnmuteVolume The volume level to increase to when unmuting.
     * directive processing exceptions to AVS.
     */
    SpeakerManager(
        const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& speakerInterfaces,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        const int minUnmuteVolume);

    /**
     * Parses the payload from a string into a rapidjson document.
     *
     * @param payload The payload as a string.
     * @param document The document that will contain the payload.
     * @return A bool indicating the results of the operation.
     */
    bool parseDirectivePayload(std::string payload, rapidjson::Document* document);

    /**
     * Performs clean-up after a successful handling of a directive.
     *
     * @param info The current directive being processed.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Removes the directive after it's been processed.
     *
     * @param info The current directive being processed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Sends an exception to AVS.
     *
     * @param info The current directive being processed.
     * @param message The exception message.
     * @param type The type of exception.
     */
    void sendExceptionEncountered(
        std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const std::string& message,
        avsCommon::avs::ExceptionErrorType type);

    /**
     * Function to update the state of the ContextManager.
     *
     * @param type The Speaker Type that is being updated.
     * @param settings The SpeakerSettings to update the ContextManager with.
     * @return Whether the ContextManager was successfully updated.
     */
    bool updateContextManager(
        const avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings);

    /**
     * Sends <Volume/Mute>Changed events to AVS. The events are identical except for the name.
     *
     * @param eventName The name of the event.
     * @param settings The current speaker settings.
     */
    void executeSendSpeakerSettingsChangedEvent(
        const std::string& eventName,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings settings);

    /**
     * Function to set the volume for a specific @c Type. This runs on a worker thread.
     * Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of speaker to modify volume for.
     * @param volume The volume to change.
     * @param forceNoNotifications This flag will ensure no event is sent and the observer is not notified.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A bool indicating success.
     */
    bool executeSetVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t volume,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API);

    /**
     * Function to restore the volume from a mute state. This runs on a worker thread and will not send an event or
     * notify an observer. Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of speaker to modify volume for.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A bool indicating success.
     */
    bool executeRestoreVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source);

    /**
     * Function to adjust the volume for a specific @c Type. This runs on a worker thread.
     * Upon success, a VolumeChanged event will be sent to AVS.
     *
     * @param type The type of speaker to modify volume for.
     * @param delta The delta to change the volume by.
     * @param forceNoNotifications This flag will ensure no event is sent and the observer is not notified.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A bool indicating success.
     */
    bool executeAdjustVolume(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        int8_t delta,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API);

    /**
     * Function to set the mute for a specific @c Type. This runs on a worker thread.
     * Upon success, a MuteChanged event will be sent to AVS.
     *
     * @param type The type of speaker to modify mute for.
     * @param mute Whether to mute/unmute.
     * @param forceNoNotifications This flag will ensure no event is sent and the observer is not notified.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @return A bool indicating success.
     */
    bool executeSetMute(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        bool mute,
        bool forceNoNotifications = false,
        avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source source =
            avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source::LOCAL_API);

    /**
     * Function to get the speaker settings for a specific @c Type.
     * This runs on a worker thread.
     *
     * @param type The type of speaker to modify mute for.
     * @param[out] settings The settings if successful.
     * @return A bool indicating success.
     */
    bool executeGetSpeakerSettings(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings);

    /**
     * Function to send events and notify observers when settings have changed.
     * This runs on a worker thread.
     *
     * @param settings The new settings.
     * @param eventName The event name to send.
     * @param source Whether the call is a result from an AVS directive or local interaction.
     * @param type The Speaker type.
     */
    void executeNotifySettingsChanged(
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings,
        const std::string& eventName,
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::SpeakerInterface::Type& type);

    /**
     * Function to notify the observer when a @c SpeakerSettings change has occurred.
     *
     * @param source. This indicates the origin of the call.
     * @param type. This indicates the type of speaker that was modified.
     * @param settings. This indicates the current settings after the change.
     */
    void executeNotifyObserver(
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings);

    /**
     * Validates that all speakers with the given @c Type have the same @c SpeakerSettings.
     *
     * @param type The type of speaker to validate.
     * @param[out] settings The settings that will be return if consistent.
     * @return A bool indicating success.
     */
    bool validateSpeakerSettingsConsistency(
        avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings* settings);

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// the @c volume to restore to when unmuting at 0 volume
    const int m_minUnmuteVolume;

    /// A multimap contain speakers keyed by @c Type.
    std::multimap<
        avsCommon::sdkInterfaces::SpeakerInterface::Type,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
        m_speakerMap;

    /// The observers to be notified whenever any of the @c SpeakerSetting changing APIs are called.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerObserverInterface>> m_observers;

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// An executor to perform operations on a worker thread.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace speakerManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_INCLUDE_SPEAKERMANAGER_SPEAKERMANAGER_H_