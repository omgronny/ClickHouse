#include <Common/typeid_cast.h>
#include <Parsers/ParserStringAndSubstitution.h>
#include <Parsers/ParserAlterQuery.h>
#include <Parsers/CommonParsers.h>
#include <Parsers/ExpressionElementParsers.h>
#include <Parsers/ExpressionListParsers.h>
#include <Parsers/ParserCreateQuery.h>
#include <Parsers/ParserPartition.h>
#include <Parsers/ParserRefreshStrategy.h>
#include <Parsers/ParserSelectWithUnionQuery.h>
#include <Parsers/ParserSetQuery.h>
#include <Parsers/ASTIdentifier.h>
#include <Parsers/ASTIndexDeclaration.h>
#include <Parsers/ASTAlterQuery.h>
#include <Parsers/ASTLiteral.h>
#include <Parsers/parseDatabaseAndTableName.h>


namespace DB
{

bool ParserAlterCommand::parseImpl(Pos & pos, ASTPtr & node, Expected & expected)
{
    auto command = std::make_shared<ASTAlterCommand>();
    node = command;

    ParserKeyword s_add_column("ADD COLUMN");
    ParserKeyword s_drop_column("DROP COLUMN");
    ParserKeyword s_clear_column("CLEAR COLUMN");
    ParserKeyword s_modify_column("MODIFY COLUMN");
    ParserKeyword s_alter_column("ALTER COLUMN");
    ParserKeyword s_rename_column("RENAME COLUMN");
    ParserKeyword s_comment_column("COMMENT COLUMN");
    ParserKeyword s_materialize_column("MATERIALIZE COLUMN");

    ParserKeyword s_modify_order_by("MODIFY ORDER BY");
    ParserKeyword s_modify_sample_by("MODIFY SAMPLE BY");
    ParserKeyword s_modify_ttl("MODIFY TTL");
    ParserKeyword s_materialize_ttl("MATERIALIZE TTL");
    ParserKeyword s_modify_setting("MODIFY SETTING");
    ParserKeyword s_reset_setting("RESET SETTING");
    ParserKeyword s_modify_query("MODIFY QUERY");
    ParserKeyword s_modify_refresh("MODIFY REFRESH");

    ParserKeyword s_add_index("ADD INDEX");
    ParserKeyword s_drop_index("DROP INDEX");
    ParserKeyword s_clear_index("CLEAR INDEX");
    ParserKeyword s_materialize_index("MATERIALIZE INDEX");

    ParserKeyword s_add_statistic("ADD STATISTIC");
    ParserKeyword s_drop_statistic("DROP STATISTIC");
    ParserKeyword s_clear_statistic("CLEAR STATISTIC");
    ParserKeyword s_materialize_statistic("MATERIALIZE STATISTIC");

    ParserKeyword s_add_constraint("ADD CONSTRAINT");
    ParserKeyword s_drop_constraint("DROP CONSTRAINT");

    ParserKeyword s_add_projection("ADD PROJECTION");
    ParserKeyword s_drop_projection("DROP PROJECTION");
    ParserKeyword s_clear_projection("CLEAR PROJECTION");
    ParserKeyword s_materialize_projection("MATERIALIZE PROJECTION");
    ParserKeyword s_modify_comment("MODIFY COMMENT");

    ParserKeyword s_add("ADD");
    ParserKeyword s_drop("DROP");
    ParserKeyword s_suspend("SUSPEND");
    ParserKeyword s_resume("RESUME");
    ParserKeyword s_refresh("REFRESH");
    ParserKeyword s_modify("MODIFY");

    ParserKeyword s_attach_partition("ATTACH PARTITION");
    ParserKeyword s_attach_part("ATTACH PART");
    ParserKeyword s_detach_partition("DETACH PARTITION");
    ParserKeyword s_detach_part("DETACH PART");
    ParserKeyword s_drop_partition("DROP PARTITION");
    ParserKeyword s_drop_part("DROP PART");
    ParserKeyword s_forget_partition("FORGET PARTITION");
    ParserKeyword s_move_partition("MOVE PARTITION");
    ParserKeyword s_move_part("MOVE PART");
    ParserKeyword s_drop_detached_partition("DROP DETACHED PARTITION");
    ParserKeyword s_drop_detached_part("DROP DETACHED PART");
    ParserKeyword s_fetch_partition("FETCH PARTITION");
    ParserKeyword s_fetch_part("FETCH PART");
    ParserKeyword s_replace_partition("REPLACE PARTITION");
    ParserKeyword s_freeze("FREEZE");
    ParserKeyword s_unfreeze("UNFREEZE");
    ParserKeyword s_partition("PARTITION");

    ParserKeyword s_first("FIRST");
    ParserKeyword s_after("AFTER");
    ParserKeyword s_if_not_exists("IF NOT EXISTS");
    ParserKeyword s_if_exists("IF EXISTS");
    ParserKeyword s_from("FROM");
    ParserKeyword s_in_partition("IN PARTITION");
    ParserKeyword s_with("WITH");
    ParserKeyword s_name("NAME");

    ParserKeyword s_to_disk("TO DISK");
    ParserKeyword s_to_volume("TO VOLUME");
    ParserKeyword s_to_table("TO TABLE");
    ParserKeyword s_to_shard("TO SHARD");

    ParserKeyword s_delete("DELETE");
    ParserKeyword s_update("UPDATE");
    ParserKeyword s_where("WHERE");
    ParserKeyword s_to("TO");

    ParserKeyword s_remove("REMOVE");
    ParserKeyword s_default("DEFAULT");
    ParserKeyword s_materialized("MATERIALIZED");
    ParserKeyword s_alias("ALIAS");
    ParserKeyword s_comment("COMMENT");
    ParserKeyword s_codec("CODEC");
    ParserKeyword s_ttl("TTL");
    ParserKeyword s_settings("SETTINGS");

    ParserKeyword s_remove_ttl("REMOVE TTL");
    ParserKeyword s_remove_sample_by("REMOVE SAMPLE BY");
    ParserKeyword s_apply_deleted_mask("APPLY DELETED MASK");

    ParserToken parser_opening_round_bracket(TokenType::OpeningRoundBracket);
    ParserToken parser_closing_round_bracket(TokenType::ClosingRoundBracket);

    ParserCompoundIdentifier parser_name;
    ParserStringLiteral parser_string_literal;
    ParserStringAndSubstitution parser_string_and_substituion;
    ParserIdentifier parser_remove_property;
    ParserCompoundColumnDeclaration parser_col_decl;
    ParserIndexDeclaration parser_idx_decl;
    ParserStatisticDeclaration parser_stat_decl;
    ParserConstraintDeclaration parser_constraint_decl;
    ParserProjectionDeclaration parser_projection_decl;
    ParserCompoundColumnDeclaration parser_modify_col_decl(false, false, true);
    ParserPartition parser_partition;
    ParserExpression parser_exp_elem;
    ParserList parser_assignment_list(
        std::make_unique<ParserAssignment>(), std::make_unique<ParserToken>(TokenType::Comma),
        /* allow_empty = */ false);
    ParserSetQuery parser_settings(true);
    ParserList parser_reset_setting(
        std::make_unique<ParserIdentifier>(), std::make_unique<ParserToken>(TokenType::Comma),
        /* allow_empty = */ false);
    ParserNameList values_p;
    ParserSelectWithUnionQuery select_p;
    ParserRefreshStrategy refresh_p;
    ParserTTLExpressionList parser_ttl_list;

    ASTPtr command_col_decl;
    ASTPtr command_column;
    ASTPtr command_order_by;
    ASTPtr command_sample_by;
    ASTPtr command_index_decl;
    ASTPtr command_index;
    ASTPtr command_constraint_decl;
    ASTPtr command_constraint;
    ASTPtr command_projection_decl;
    ASTPtr command_projection;
    ASTPtr command_statistic_decl;
    ASTPtr command_partition;
    ASTPtr command_predicate;
    ASTPtr command_update_assignments;
    ASTPtr command_comment;
    ASTPtr command_ttl;
    ASTPtr command_settings_changes;
    ASTPtr command_settings_resets;
    ASTPtr command_select;
    ASTPtr command_values;
    ASTPtr command_rename_to;

    if (with_round_bracket)
    {
        if (!parser_opening_round_bracket.ignore(pos, expected))
            return false;
    }

    switch (alter_object)
    {
        case ASTAlterQuery::AlterObjectType::LIVE_VIEW:
        {
            if (s_refresh.ignore(pos, expected))
            {
                command->type = ASTAlterCommand::LIVE_VIEW_REFRESH;
            }
            else
                return false;
            break;
        }
        case ASTAlterQuery::AlterObjectType::DATABASE:
        {
            if (s_modify_setting.ignore(pos, expected))
            {
                if (!parser_settings.parse(pos, command_settings_changes, expected))
                    return false;
                command->type = ASTAlterCommand::MODIFY_DATABASE_SETTING;
            }
            else
                return false;
            break;
        }
        default:
            break;
        case ASTAlterQuery::AlterObjectType::TABLE:
        {
            if (s_add_column.ignore(pos, expected))
            {
                if (s_if_not_exists.ignore(pos, expected))
                    command->if_not_exists = true;

                if (!parser_col_decl.parse(pos, command_col_decl, expected))
                    return false;

                if (s_first.ignore(pos, expected))
                    command->first = true;
                else if (s_after.ignore(pos, expected))
                {
                    if (!parser_name.parse(pos, command_column, expected))
                        return false;
                }

                command->type = ASTAlterCommand::ADD_COLUMN;
            }
            else if (s_rename_column.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_column, expected))
                    return false;

                if (!s_to.ignore(pos, expected))
                    return false;

                if (!parser_name.parse(pos, command_rename_to, expected))
                    return false;

                command->type = ASTAlterCommand::RENAME_COLUMN;
            }
            else if (s_materialize_column.ignore(pos, expected))
            {
                if (!parser_name.parse(pos, command_column, expected))
                    return false;

                command->type = ASTAlterCommand::MATERIALIZE_COLUMN;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_drop_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PARTITION;
            }
            else if (s_drop_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PARTITION;
                command->part = true;
            }
            else if (s_forget_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::FORGET_PARTITION;
            }
            else if (s_drop_detached_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_DETACHED_PARTITION;
            }
            else if (s_drop_detached_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_DETACHED_PARTITION;
                command->part = true;
            }
            else if (s_drop_column.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_column, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_COLUMN;
                command->detach = false;
            }
            else if (s_clear_column.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_column, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_COLUMN;
                command->clear_column = true;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_add_index.ignore(pos, expected))
            {
                if (s_if_not_exists.ignore(pos, expected))
                    command->if_not_exists = true;

                if (!parser_idx_decl.parse(pos, command_index_decl, expected))
                    return false;

                if (s_first.ignore(pos, expected))
                    command->first = true;
                else if (s_after.ignore(pos, expected))
                {
                    if (!parser_name.parse(pos, command_index, expected))
                        return false;
                }

                command->type = ASTAlterCommand::ADD_INDEX;
            }
            else if (s_drop_index.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_index, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_INDEX;
                command->detach = false;
            }
            else if (s_clear_index.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_index, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_INDEX;
                command->clear_index = true;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_materialize_index.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_index, expected))
                    return false;

                command->type = ASTAlterCommand::MATERIALIZE_INDEX;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_add_statistic.ignore(pos, expected))
            {
                if (s_if_not_exists.ignore(pos, expected))
                    command->if_not_exists = true;

                if (!parser_stat_decl.parse(pos, command_statistic_decl, expected))
                    return false;

                command->type = ASTAlterCommand::ADD_STATISTIC;
            }
            else if (s_drop_statistic.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_stat_decl.parse(pos, command_statistic_decl, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_STATISTIC;
            }
            else if (s_clear_statistic.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_stat_decl.parse(pos, command_statistic_decl, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_STATISTIC;
                command->clear_statistic = true;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_materialize_statistic.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_stat_decl.parse(pos, command_statistic_decl, expected))
                    return false;

                command->type = ASTAlterCommand::MATERIALIZE_STATISTIC;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_add_projection.ignore(pos, expected))
            {
                if (s_if_not_exists.ignore(pos, expected))
                    command->if_not_exists = true;

                if (!parser_projection_decl.parse(pos, command_projection_decl, expected))
                    return false;

                if (s_first.ignore(pos, expected))
                    command->first = true;
                else if (s_after.ignore(pos, expected))
                {
                    if (!parser_name.parse(pos, command_projection, expected))
                        return false;
                }

                command->type = ASTAlterCommand::ADD_PROJECTION;
            }
            else if (s_drop_projection.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_projection, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PROJECTION;
                command->detach = false;
            }
            else if (s_clear_projection.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_projection, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PROJECTION;
                command->clear_projection = true;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_materialize_projection.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_projection, expected))
                    return false;

                command->type = ASTAlterCommand::MATERIALIZE_PROJECTION;
                command->detach = false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_move_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::MOVE_PARTITION;
                command->part = true;

                if (s_to_disk.ignore(pos))
                    command->move_destination_type = DataDestinationType::DISK;
                else if (s_to_volume.ignore(pos))
                    command->move_destination_type = DataDestinationType::VOLUME;
                else if (s_to_shard.ignore(pos))
                {
                    command->move_destination_type = DataDestinationType::SHARD;
                }
                else
                    return false;

                ASTPtr ast_space_name;
                if (!parser_string_literal.parse(pos, ast_space_name, expected))
                    return false;

                command->move_destination_name = ast_space_name->as<ASTLiteral &>().value.get<const String &>();
            }
            else if (s_move_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::MOVE_PARTITION;

                if (s_to_disk.ignore(pos))
                    command->move_destination_type = DataDestinationType::DISK;
                else if (s_to_volume.ignore(pos))
                    command->move_destination_type = DataDestinationType::VOLUME;
                else if (s_to_table.ignore(pos))
                {
                    if (!parseDatabaseAndTableName(pos, expected, command->to_database, command->to_table))
                        return false;
                    command->move_destination_type = DataDestinationType::TABLE;
                }
                else
                    return false;

                if (command->move_destination_type != DataDestinationType::TABLE)
                {
                    ASTPtr ast_space_name;
                    if (!parser_string_literal.parse(pos, ast_space_name, expected))
                        return false;

                    command->move_destination_name = ast_space_name->as<ASTLiteral &>().value.get<const String &>();
                }
            }
            else if (s_add_constraint.ignore(pos, expected))
            {
                if (s_if_not_exists.ignore(pos, expected))
                    command->if_not_exists = true;

                if (!parser_constraint_decl.parse(pos, command_constraint_decl, expected))
                    return false;

                command->type = ASTAlterCommand::ADD_CONSTRAINT;
            }
            else if (s_drop_constraint.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_constraint, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_CONSTRAINT;
                command->detach = false;
            }
            else if (s_detach_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PARTITION;
                command->detach = true;
            }
            else if (s_detach_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                command->type = ASTAlterCommand::DROP_PARTITION;
                command->part = true;
                command->detach = true;
            }
            else if (s_attach_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                if (s_from.ignore(pos))
                {
                    if (!parseDatabaseAndTableName(pos, expected, command->from_database, command->from_table))
                        return false;

                    command->replace = false;
                    command->type = ASTAlterCommand::REPLACE_PARTITION;
                }
                else
                {
                    command->type = ASTAlterCommand::ATTACH_PARTITION;
                }
            }
            else if (s_replace_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                if (!s_from.ignore(pos, expected))
                    return false;

                if (!parseDatabaseAndTableName(pos, expected, command->from_database, command->from_table))
                    return false;

                command->replace = true;
                command->type = ASTAlterCommand::REPLACE_PARTITION;
            }
            else if (s_attach_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                command->part = true;
                command->type = ASTAlterCommand::ATTACH_PARTITION;
            }
            else if (s_fetch_partition.ignore(pos, expected))
            {
                if (!parser_partition.parse(pos, command_partition, expected))
                    return false;

                if (!s_from.ignore(pos, expected))
                    return false;

                ASTPtr ast_from;
                if (!parser_string_literal.parse(pos, ast_from, expected))
                    return false;

                command->from = ast_from->as<ASTLiteral &>().value.get<const String &>();
                command->type = ASTAlterCommand::FETCH_PARTITION;
            }
            else if (s_fetch_part.ignore(pos, expected))
            {
                if (!parser_string_and_substituion.parse(pos, command_partition, expected))
                    return false;

                if (!s_from.ignore(pos, expected))
                    return false;

                ASTPtr ast_from;
                if (!parser_string_literal.parse(pos, ast_from, expected))
                    return false;
                command->from = ast_from->as<ASTLiteral &>().value.get<const String &>();
                command->part = true;
                command->type = ASTAlterCommand::FETCH_PARTITION;
            }
            else if (s_freeze.ignore(pos, expected))
            {
                if (s_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;

                    command->type = ASTAlterCommand::FREEZE_PARTITION;
                }
                else
                {
                    command->type = ASTAlterCommand::FREEZE_ALL;
                }

                /// WITH NAME 'name' - place local backup to directory with specified name
                if (s_with.ignore(pos, expected))
                {
                    if (!s_name.ignore(pos, expected))
                        return false;

                    ASTPtr ast_with_name;
                    if (!parser_string_literal.parse(pos, ast_with_name, expected))
                        return false;

                    command->with_name = ast_with_name->as<ASTLiteral &>().value.get<const String &>();
                }
            }
            else if (s_unfreeze.ignore(pos, expected))
            {
                if (s_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;

                    command->type = ASTAlterCommand::UNFREEZE_PARTITION;
                }
                else
                {
                    command->type = ASTAlterCommand::UNFREEZE_ALL;
                }

                /// WITH NAME 'name' - remove local backup to directory with specified name
                if (s_with.ignore(pos, expected))
                {
                    if (!s_name.ignore(pos, expected))
                        return false;

                    ASTPtr ast_with_name;
                    if (!parser_string_literal.parse(pos, ast_with_name, expected))
                        return false;

                    command->with_name = ast_with_name->as<ASTLiteral &>().value.get<const String &>();
                }
                else
                {
                    return false;
                }
            }
            else if (bool is_modify = s_modify_column.ignore(pos, expected); is_modify || s_alter_column.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!is_modify)
                    parser_modify_col_decl.enableCheckTypeKeyword();

                if (!parser_modify_col_decl.parse(pos, command_col_decl, expected))
                    return false;

                if (s_remove.ignore(pos, expected))
                {
                    if (s_default.ignore(pos, expected))
                        command->remove_property = "DEFAULT";
                    else if (s_materialized.ignore(pos, expected))
                        command->remove_property = "MATERIALIZED";
                    else if (s_alias.ignore(pos, expected))
                        command->remove_property = "ALIAS";
                    else if (s_comment.ignore(pos, expected))
                        command->remove_property = "COMMENT";
                    else if (s_codec.ignore(pos, expected))
                        command->remove_property = "CODEC";
                    else if (s_ttl.ignore(pos, expected))
                        command->remove_property = "TTL";
                    else if (s_settings.ignore(pos, expected))
                        command->remove_property = "SETTINGS";
                    else
                        return false;
                }
                else if (s_modify_setting.ignore(pos, expected))
                {
                    if (!parser_settings.parse(pos, command_settings_changes, expected))
                        return false;
                }
                else if (s_reset_setting.ignore(pos, expected))
                {
                    if (!parser_reset_setting.parse(pos, command_settings_resets, expected))
                        return false;
                }
                else
                {
                    if (s_first.ignore(pos, expected))
                        command->first = true;
                    else if (s_after.ignore(pos, expected))
                    {
                        if (!parser_name.parse(pos, command_column, expected))
                            return false;
                    }
                }
                command->type = ASTAlterCommand::MODIFY_COLUMN;
            }
            else if (s_modify_order_by.ignore(pos, expected))
            {
                if (!parser_exp_elem.parse(pos, command_order_by, expected))
                    return false;

                command->type = ASTAlterCommand::MODIFY_ORDER_BY;
            }
            else if (s_modify_sample_by.ignore(pos, expected))
            {
                if (!parser_exp_elem.parse(pos, command_sample_by, expected))
                    return false;

                command->type = ASTAlterCommand::MODIFY_SAMPLE_BY;
            }
            else if (s_remove_sample_by.ignore(pos, expected))
            {
                command->type = ASTAlterCommand::REMOVE_SAMPLE_BY;
            }
            else if (s_delete.ignore(pos, expected))
            {
                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }

                if (!s_where.ignore(pos, expected))
                    return false;

                if (!parser_exp_elem.parse(pos, command_predicate, expected))
                    return false;

                command->type = ASTAlterCommand::DELETE;
            }
            else if (s_update.ignore(pos, expected))
            {
                if (!parser_assignment_list.parse(pos, command_update_assignments, expected))
                    return false;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }

                if (!s_where.ignore(pos, expected))
                    return false;

                if (!parser_exp_elem.parse(pos, command_predicate, expected))
                    return false;

                command->type = ASTAlterCommand::UPDATE;
            }
            else if (s_comment_column.ignore(pos, expected))
            {
                if (s_if_exists.ignore(pos, expected))
                    command->if_exists = true;

                if (!parser_name.parse(pos, command_column, expected))
                    return false;

                if (!parser_string_literal.parse(pos, command_comment, expected))
                    return false;

                command->type = ASTAlterCommand::COMMENT_COLUMN;
            }
            else if (s_modify_ttl.ignore(pos, expected))
            {
                if (!parser_ttl_list.parse(pos, command_ttl, expected))
                    return false;
                command->type = ASTAlterCommand::MODIFY_TTL;
            }
            else if (s_remove_ttl.ignore(pos, expected))
            {
                command->type = ASTAlterCommand::REMOVE_TTL;
            }
            else if (s_materialize_ttl.ignore(pos, expected))
            {
                command->type = ASTAlterCommand::MATERIALIZE_TTL;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else if (s_modify_setting.ignore(pos, expected))
            {
                if (!parser_settings.parse(pos, command_settings_changes, expected))
                    return false;
                command->type = ASTAlterCommand::MODIFY_SETTING;
            }
            else if (s_reset_setting.ignore(pos, expected))
            {
                if (!parser_reset_setting.parse(pos, command_settings_resets, expected))
                    return false;
                command->type = ASTAlterCommand::RESET_SETTING;
            }
            else if (s_modify_query.ignore(pos, expected))
            {
                if (!select_p.parse(pos, command_select, expected))
                    return false;
                command->type = ASTAlterCommand::MODIFY_QUERY;
            }
            else if (s_modify_refresh.ignore(pos, expected))
            {
                if (!refresh_p.parse(pos, command->refresh, expected))
                    return false;
                command->type = ASTAlterCommand::MODIFY_REFRESH;
            }
            else if (s_modify_comment.ignore(pos, expected))
            {
                if (!parser_string_literal.parse(pos, command_comment, expected))
                    return false;

                command->type = ASTAlterCommand::MODIFY_COMMENT;
            }
            else if (s_apply_deleted_mask.ignore(pos, expected))
            {
                command->type = ASTAlterCommand::APPLY_DELETED_MASK;

                if (s_in_partition.ignore(pos, expected))
                {
                    if (!parser_partition.parse(pos, command_partition, expected))
                        return false;
                }
            }
            else
                return false;
        }
    }

    if (with_round_bracket)
    {
        if (!parser_closing_round_bracket.ignore(pos, expected))
            return false;
    }

    if (command_col_decl)
        command->col_decl = command->children.emplace_back(std::move(command_col_decl)).get();
    if (command_column)
        command->column = command->children.emplace_back(std::move(command_column)).get();
    if (command_order_by)
        command->order_by = command->children.emplace_back(std::move(command_order_by)).get();
    if (command_sample_by)
        command->sample_by = command->children.emplace_back(std::move(command_sample_by)).get();
    if (command_index_decl)
        command->index_decl = command->children.emplace_back(std::move(command_index_decl)).get();
    if (command_index)
        command->index = command->children.emplace_back(std::move(command_index)).get();
    if (command_constraint_decl)
        command->constraint_decl = command->children.emplace_back(std::move(command_constraint_decl)).get();
    if (command_constraint)
        command->constraint = command->children.emplace_back(std::move(command_constraint)).get();
    if (command_projection_decl)
        command->projection_decl = command->children.emplace_back(std::move(command_projection_decl)).get();
    if (command_projection)
        command->projection = command->children.emplace_back(std::move(command_projection)).get();
    if (command_statistic_decl)
        command->statistic_decl = command->children.emplace_back(std::move(command_statistic_decl)).get();
    if (command_partition)
        command->partition = command->children.emplace_back(std::move(command_partition)).get();
    if (command_predicate)
        command->predicate = command->children.emplace_back(std::move(command_predicate)).get();
    if (command_update_assignments)
        command->update_assignments = command->children.emplace_back(std::move(command_update_assignments)).get();
    if (command_comment)
        command->comment = command->children.emplace_back(std::move(command_comment)).get();
    if (command_ttl)
        command->ttl = command->children.emplace_back(std::move(command_ttl)).get();
    if (command_settings_changes)
        command->settings_changes = command->children.emplace_back(std::move(command_settings_changes)).get();
    if (command_settings_resets)
        command->settings_resets = command->children.emplace_back(std::move(command_settings_resets)).get();
    if (command_select)
        command->select = command->children.emplace_back(std::move(command_select)).get();
    if (command_values)
        command->values = command->children.emplace_back(std::move(command_values)).get();
    if (command_rename_to)
        command->rename_to = command->children.emplace_back(std::move(command_rename_to)).get();

    return true;
}


bool ParserAlterCommandList::parseImpl(Pos & pos, ASTPtr & node, Expected & expected)
{
    auto command_list = std::make_shared<ASTExpressionList>();
    node = command_list;

    ParserToken s_comma(TokenType::Comma);

    const auto with_round_bracket = pos->type == TokenType::OpeningRoundBracket;

    ParserAlterCommand p_command(with_round_bracket, alter_object);

    do
    {
        ASTPtr command;
        if (!p_command.parse(pos, command, expected))
            return false;

        command_list->children.push_back(command);
    }
    while (s_comma.ignore(pos, expected));

    return true;
}

bool ParserAlterQuery::parseImpl(Pos & pos, ASTPtr & node, Expected & expected)
{
    auto query = std::make_shared<ASTAlterQuery>();
    node = query;

    ParserKeyword s_alter_table("ALTER TABLE");
    ParserKeyword s_alter_temporary_table("ALTER TEMPORARY TABLE");
    ParserKeyword s_alter_live_view("ALTER LIVE VIEW");
    ParserKeyword s_alter_database("ALTER DATABASE");

    ASTAlterQuery::AlterObjectType alter_object_type;

    if (s_alter_table.ignore(pos, expected) || s_alter_temporary_table.ignore(pos, expected))
    {
        alter_object_type = ASTAlterQuery::AlterObjectType::TABLE;
    }
    else if (s_alter_live_view.ignore(pos, expected))
    {
        alter_object_type = ASTAlterQuery::AlterObjectType::LIVE_VIEW;
    }
    else if (s_alter_database.ignore(pos, expected))
    {
        alter_object_type = ASTAlterQuery::AlterObjectType::DATABASE;
    }
    else
        return false;

    if (alter_object_type == ASTAlterQuery::AlterObjectType::DATABASE)
    {
        if (!parseDatabaseAsAST(pos, expected, query->database))
            return false;
    }
    else
    {
        if (!parseDatabaseAndTableAsAST(pos, expected, query->database, query->table))
            return false;

        String cluster_str;
        if (ParserKeyword{"ON"}.ignore(pos, expected))
        {
            if (!ASTQueryWithOnCluster::parse(pos, cluster_str, expected))
                return false;
        }
        query->cluster = cluster_str;
    }

    ParserAlterCommandList p_command_list(alter_object_type);
    ASTPtr command_list;
    if (!p_command_list.parse(pos, command_list, expected))
        return false;

    query->set(query->command_list, command_list);
    query->alter_object = alter_object_type;

    if (query->database)
        query->children.push_back(query->database);

    if (query->table)
        query->children.push_back(query->table);

    return true;
}

}
