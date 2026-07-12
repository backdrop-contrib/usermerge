<?php
/**
 * @file
 * Admin pages for the User Merge module.
 */

/**
 * Form builder for User Merge settings.
 */
function usermerge_settings_form() {
  $form['core'] = array(
    '#type' => 'fieldset',
    '#title' => t('Core properties'),
    '#description' => t('By default, the module handles core properties automatically, by keeping the value from the user to keep. This is why most of them do not appear in the review table. If you want to display some core properties in the review table, enable them here by checking the relevant boxes.'),
    '#tree' => TRUE,
  );

  $usermerge_settings = config_get('usermerge.settings', 'usermerge_settings');
  $core_properties = usermerge_get_user_core_properties();
  foreach ($core_properties as $property) {
    // For some properties there is never a choice of whether to display them in
    // the review table.
    $no_choice_properties = array('uid', 'created', 'access', 'login', 'pass', 'init', 'data', 'roles');
    if (!in_array($property, $no_choice_properties)) {
      $form['core'][$property] = array(
        '#type' => 'checkbox',
        '#title' => $property, // No need to localize this
        '#default_value' => !empty($usermerge_settings['core'][$property]) ? $usermerge_settings['core'][$property] : 0,
      );
    }
  }

  $form['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['advanced']['match_uid'] = array(
    '#type' => 'checkbox',
    '#title' => t('Match UID'),
    '#description' => t('Allow matching on the UID as well as the name of user accounts on the merge form.'),
    '#default_value' => config_get('usermerge.advanced.settings', 'match_uid'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

/**
 * Submit handler for the  User Merge settings form.
 */
function usermerge_settings_form_submit($form, &$form_state) {
  form_state_values_clean($form_state);
  foreach ($form_state['values'] as $type => $values) {
    if ($type != 'match_uid') {
      $usermerge_settings[$type] = $values;
    }
  }
  config_set('usermerge.settings', 'usermerge_settings', $usermerge_settings);
  config_set('usermerge.advanced.settings', 'match_uid', $form_state['values']['match_uid']);
  backdrop_set_message('The User Merge settings have been saved.');
}

/**
 * Returns an array of the core properties for the user entity.
 */
function usermerge_get_user_core_properties() {
  static $properties;

  if (empty($properties)) {
    // Define list of fields and other user data using the columns in the user
    // table, so non-field data added from other modules doesn't get mixed in.
    $user_entity_info = entity_get_info('user');
    $properties = $user_entity_info['schema_fields_sql']['base table'];
    // Adding roles
    $properties[] = 'roles';
  }

  return $properties;
}

/**
 * Form to collect the two user IDs.
 */
function usermerge_merge_form($form, &$form_state) {
  if (!empty($form_state['form_page']) && $form_state['form_page'] == 'review_table') {
    return usermerge_data_review_form($form, $form_state);
  }

  $form['supported_actions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Supported actions'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $supported_actions = usermerge_invoke_all('usermerge_actions_supported', array());
  ksort($supported_actions);
  $form['supported_actions']['list'] = array(
    '#theme' => 'item_list',
    '#type' => 'ul',
    '#items' => $supported_actions,
  );

  $match_uid = config_get('usermerge.advanced.settings', 'match_uid');
  $form['general']['usermerge_user_delete'] = array(
    '#type' => 'textfield',
    '#title' => $match_uid ? t('The name or uid of the account you wish to remove') : t('The name of the account you wish to remove'),
    '#autocomplete_path' => 'usermerge/autocomplete',
    '#required' => TRUE,
  );

  $form['general']['usermerge_user_keep'] = array(
    '#type' => 'textfield',
    '#title' => $match_uid ? t('The name or uid of the account you wish to keep') : t('The name of the account you wish to keep'),
    '#autocomplete_path' => 'usermerge/autocomplete',
    '#required' => TRUE,
  );

  $form['general']['usermerge_user_delete_action'] = array(
    '#type' => 'select',
    '#title' => t('Action to perform on the account you wish to remove'),
    '#options' => usermerge_remove_actions(),
    '#default_value' => 'block',
  );

  $form['general']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Review account data'),
    '#validate' => array('usermerge_merge_form_validate'),
    '#submit' => array('usermerge_merge_form_submit'),
  );

  return $form;
}

/**
 * Menu callback; retrieve a JSON object containing autocomplete suggestions for
 * existing users.
 *
 * @see user_autocomplete()
 */
function usermerge_autocomplete($string = '') {
  $matches = array();
  if ($string) {
    $match_uid = config_get('usermerge.advanced.settings', 'match_uid');
    $select = db_select('users')
      ->fields('users', array('name', 'uid'));
    if ($match_uid) {
      $select->condition(db_or()
        ->condition('name', db_like($string) . '%', 'LIKE')
        ->condition('uid', db_like($string) . '%', 'LIKE')
      );
    }
    else {
      $select->condition('name', db_like($string) . '%', 'LIKE');
    }
    $result = $select
      ->range(0, 10)
      ->execute();
    foreach ($result as $account) {
      if ($match_uid) {
        $match = $account->name . ' | ' . $account->uid;
      }
      else {
        $match = $account->name;
      }
      $matches[$match] = check_plain($match);
    }
  }

  // Displayed as JSON by delivery callback backdrop_json_deliver().
  return $matches;
}

/**
 * Validates contents of form for user validation.
 */
function usermerge_merge_form_validate($form, &$form_state) {
  // Validate first step
  if (!isset($form_state['form_page'])) {
    $user_keep_raw = $form_state['values']['usermerge_user_keep'];
    $user_delete_raw = $form_state['values']['usermerge_user_delete'];

    if (config_get('usermerge.advanced.settings', 'match_uid')) {
      $user_keep_parts = array_map('trim', explode('|', $user_keep_raw));
      $user_keep = $user_keep_parts[0];
      $user_delete_parts = array_map('trim', explode('|', $user_delete_raw));
      $user_delete = $user_delete_parts[0];
    }
    else {
      $user_keep = $user_keep_raw;
      $user_delete = $user_delete_raw;
    }

    // Can't be the same user.
    if ($user_keep == $user_delete) {
      form_set_error('usermerge_user', 'You cannot have the same username in both fields.');
    }

    $user_to_delete = user_load_multiple(array(), array('name' => $user_delete));
    $user_to_keep = user_load_multiple(array(), array('name' => $user_keep));

    // Load up the users. We store these in the form_state['values'] so that we don't have to load again later.
    $form_state['values']['user_to_delete'] = array_shift($user_to_delete);
    $form_state['values']['user_to_keep'] = array_shift($user_to_keep);

    // Use the validate helper function.
    usermerge_validate_merge($form_state['values']['user_to_delete'], $form_state['values']['user_to_keep']);
  }
}

/**
 * Form that allows the selection of data to be merged.
 */
function usermerge_data_review_form($form, &$form_state) {
  backdrop_set_title(t('Merge accounts: review account data'));

  $user_to_delete = $form_state['values']['user_to_delete'];
  $user_to_keep = $form_state['values']['user_to_keep'];
  $action = $form_state['page_values']['merge_form']['usermerge_user_delete_action'];

  $account_properties = usermerge_invoke_all('usermerge_account_properties', $user_to_delete, $user_to_keep, $action);
  // Allow modules to alter pre-existing account properties
  backdrop_alter('usermerge_account_properties', $account_properties, $user_to_delete, $user_to_keep, $action);

  $form['review'] = usermerge_invoke_all('usermerge_build_review_form_elements', array(), $account_properties, $user_to_delete, $user_to_keep);

  $form['review']['#type'] = 'container';
  $form['review']['#tree'] = TRUE;

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Merge accounts'),
  );

  $form['cancel'] = array(
    '#type' => 'markup',
    '#markup' => l(t('Cancel'), 'admin/people/merge'),
  );

  return $form;
}

/**
 * Helper validation function used by the form submit and the API.
 *
 * @param $user_to_delete
 *   Object of user to be deleted
 * @param $user_to_keep
 *   Object of user to be kept
 */
function usermerge_validate_merge($user_to_delete, $user_to_keep) {
  $valid = TRUE;
  if (empty($user_to_delete->uid)) {
    form_set_error('usermerge_user_delete', t('This user name does not exist.'));
    $valid = FALSE;
  }
  // Don't remove UID 1.
  elseif ($user_to_delete->uid == 1) {
    form_set_error('usermerge_user_delete', t('Blocking or deleting user 1 is not allowed.'));
    $valid = FALSE;
  }

  if (empty($user_to_keep->uid)) {
    form_set_error('usermerge_user_keep', t('This user name does not exist.'));
    $valid = FALSE;
  }

  return $valid;
}

/**
 * Displays the userdata-review form, or merges the selected accounts.
 */
function usermerge_merge_form_submit($form, &$form_state) {
  if (!isset($form_state['form_page'])) {
    $form_state['page_values']['merge_form'] = $form_state['values'];

    if (!empty($form_state['page_values']['review_table'])) {
      $form_state['values'] = $form_state['page_values']['review_table'];
    }

    $form_state['form_page'] = 'review_table';
    $form_state['rebuild'] = TRUE;
  }
  else {
    $form_state['values']['usermerge_user_delete_action'] = $form_state['page_values']['merge_form']['usermerge_user_delete_action'];
    usermerge_merge_accounts($form_state['page_values']['merge_form']['user_to_delete'], $form_state['page_values']['merge_form']['user_to_keep'], $form_state['values']);
  }
}

/**
 * Merges the selected accounts.
 *
 * @param object $user_to_delete
 *   A fully loaded user object from user_load() that will be merged and blocked or deleted.
 * @param object $user_to_keep
 *   A fully loaded user object from user_load() that will be merged and retained.
 * @param $form_values
 *   The values from the submitted data-review form.
 * @return boolean
 *   TRUE if they are merged, FALSE if the validation failed.
 */
function usermerge_merge_accounts($user_to_delete, $user_to_keep, $form_values) {
  module_load_include('inc', 'user', 'user.actions');
  if (usermerge_validate_merge($user_to_delete, $user_to_keep)) {

    // Allow modules to create a list of merged values.
    $merged_account = usermerge_invoke_all('usermerge_merge_accounts', $user_to_delete, $user_to_keep, $form_values['review']);

    // Allow modules to modify the array without merging values.
    backdrop_alter('usermerge_merge_accounts', $merged_account, $user_to_delete, $user_to_keep);

    // Block or delete old account. Do this before saving the new account in
    // case the merged account is using the deleted account's mail or name.
    switch ($form_values['usermerge_user_delete_action']) {
      case 'block':
        $context = array();
        user_block_user_action($user_to_delete, $context);
        break;

      case 'delete':
        user_delete($user_to_delete->uid);
        break;
    }

    // Update the account to keep with the merged properties and save.
    foreach ($merged_account as $property => $value) {
      $user_to_keep->$property = $value;
    }
    user_save($user_to_keep);

    backdrop_set_message(t('%user_to_delete was successfully merged into !user_to_keep',
      array(
        '%user_to_delete' => $user_to_delete->name,
        '!user_to_keep' => l($user_to_keep->name, "user/{$user_to_keep->uid}"))));
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/**
 * Returns a list of actions to perform on the removed user.
 */
function usermerge_remove_actions() {
return array(
    'block' => t('Block'),
    'delete' => t('Delete'),
  );
}
