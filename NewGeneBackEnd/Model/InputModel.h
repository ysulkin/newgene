#ifndef INPUTMODEL_H
#define INPUTMODEL_H

#include "Model.h"
#include "..\Settings\InputModelSettings.h"
#include "..\Settings\Setting.h"
#include "Tables/TableManager.h"

class InputModel : public Model<INPUT_MODEL_SETTINGS_NAMESPACE::INPUT_MODEL_SETTINGS>
{

	public:

		InputModel(Messager & messager, boost::filesystem::path const path_to_model_database)
			: Model(messager, path_to_model_database)
		{

		}

		void LoadTables();

		Table_VariableGroupIdentifier t_vgp_identifiers;
		//Table_VariableIdentifier t_var_identifiers;

};

#endif
