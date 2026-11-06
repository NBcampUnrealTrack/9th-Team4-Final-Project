"""Read-only inspection of the saved boss combat table's row struct and row names."""
import unreal

path = '/Game/ProjectNF/Dev/Data/Boss/DT_BossCombat'
table = unreal.load_asset(path)
if not table:
    raise RuntimeError('BOSS_TABLE_DIAG: table could not be loaded: ' + path)
row_struct = table.get_editor_property('row_struct')
unreal.log('BOSS_TABLE_DIAG: Table=' + table.get_path_name()
           + ' RowStruct=' + (row_struct.get_path_name() if row_struct else 'None'))
unreal.log('BOSS_TABLE_DIAG: Rows=' + str(unreal.DataTableFunctionLibrary.get_data_table_row_names(table)))
