#ifndef UIPROJECTSETTINGS_H
#define UIPROJECTSETTINGS_H

#include <QObject>
#include "../../../NewGeneBackEnd/Utilities/NewGeneException.h"
#include "uiallsettings.h"
#include "../../../NewGeneBackEnd/Settings/ProjectSettings.h"

template<typename BACKEND_PROJECT_CLASS, typename BACKEND_PROJECT_SETTINGS_CLASS, typename PROJECT_SETTINGS_ENUM, typename BACKEND_PROJECT_SETTING_CLASS, typename UI_PROJECT_SETTING_CLASS>
class UIAllProjectSettings : public UIAllSettings
{

	public:

		UIAllProjectSettings(Messager & messager, BACKEND_PROJECT_CLASS & project, boost::filesystem::path const path_to_settings = boost::filesystem::path(), QObject * parent = NULL)
			: UIAllSettings(messager, parent)
		{
			CreateImplementation(messager, project, path_to_settings);
		}

		// The following is the equivalent of the backend's ProjectSettings class
		class UIOnlySettings : public UIOnlySettings_base<PROJECT_SETTINGS_ENUM, UI_PROJECT_SETTING_CLASS>
		{

			public:

				UIOnlySettings(Messager & messager, boost::filesystem::path const path_to_settings = boost::filesystem::path()) : UIOnlySettings_base(messager, path_to_settings)
				{

				}


			protected:

				boost::filesystem::path GetSettingsPath(Messager & messager, SettingInfo & setting_info)
				{
					return boost::filesystem::path();
				}

		};

	protected:

		class _impl : public _impl_base<BACKEND_PROJECT_SETTINGS_CLASS, UIOnlySettings>
		{

			public:

				_impl(Messager & messager, BACKEND_PROJECT_CLASS & project, boost::filesystem::path const path_to_settings = boost::filesystem::path()) : _impl_base(messager)
				{
					CreateInternalImplementations(messager, project, path_to_settings);
				}

				class _UIRelatedImpl : public _UIProjectRelatedImpl_base
				{

					public:

						_UIRelatedImpl(Messager & messager, boost::filesystem::path const path_to_settings = boost::filesystem::path())
							: _RelatedImpl_base<UIOnlySettings>(messager, path_to_settings)
							, _UIProjectRelatedImpl_base(messager, path_to_settings)
						{

						}

				};

				class _BackendRelatedImpl : public _BackendProjectRelatedImpl_base<BACKEND_PROJECT_CLASS>
				{

					public:

						_BackendRelatedImpl(Messager & messager, BACKEND_PROJECT_CLASS & project, boost::filesystem::path const path_to_settings = boost::filesystem::path())
							: _RelatedImpl_base<BACKEND_PROJECT_SETTINGS_CLASS>(messager, path_to_settings)
							, _BackendProjectRelatedImpl_base(messager, project, path_to_settings)
						{

						}

				};

			protected:

				void CreateInternalImplementations(Messager & messager, BACKEND_PROJECT_CLASS & project, boost::filesystem::path const path_to_settings = boost::filesystem::path())
				{
					__ui_impl.reset(new _UIRelatedImpl(messager, path_to_settings));
					__backend_impl.reset(new _BackendRelatedImpl(messager, project, path_to_settings));
				}

		};

		_impl_base<BACKEND_PROJECT_SETTINGS_CLASS, UIOnlySettings> & getImplementation()
		{
			if (!__impl)
			{
				boost::format msg( "UI Project settings implementation not yet constructed." );
				throw NewGeneException() << newgene_error_description( msg.str() );
			}
			return *__impl;
		}

		void CreateImplementation(Messager & messager, BACKEND_PROJECT_CLASS & project, boost::filesystem::path const path_to_settings = boost::filesystem::path())
		{
			__impl.reset(new _impl(messager, project, path_to_settings));
		}

		std::unique_ptr<_impl> __impl;


	public:

		UIOnlySettings & getUISettings()
		{
			if (!__impl)
			{
				boost::format msg( "Internal settings implementation not yet constructed." );
				throw NewGeneException() << newgene_error_description( msg.str() );
			}
			return reinterpret_cast<UIOnlySettings &>(getUISettings_base<BACKEND_PROJECT_SETTINGS_CLASS, UIOnlySettings, PROJECT_SETTINGS_ENUM, UI_PROJECT_SETTING_CLASS>(*__impl));
		}

		BACKEND_PROJECT_SETTINGS_CLASS & getBackendSettings()
		{
			if (!__impl)
			{
				boost::format msg( "Internal settings implementation not yet constructed." );
				throw NewGeneException() << newgene_error_description( msg.str() );
			}
			return reinterpret_cast<BACKEND_PROJECT_SETTINGS_CLASS &>(getBackendSettings_base<BACKEND_PROJECT_SETTINGS_CLASS, UIOnlySettings, PROJECT_SETTINGS_ENUM, BACKEND_PROJECT_SETTING_CLASS>(*__impl));
		}

};

#endif // UIPROJECTSETTINGS_H
