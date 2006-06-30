<?php
// $Id: parser.inc,v 1.16.2.1 2006/06/30 13:12:13 unconed Exp $

/**
 * @file
 * The PHP documentation parser that generates content for api.module.
 */

/**
 * Parse out function definitions from the PHP manual.
 */
function api_parse_php_manual($location) {
  $response = drupal_http_request($location);
  if ($response->code == 200) {
    $function_matches = array();
    preg_match_all('!^[a-zA-Z0-9_]+ ([a-zA-Z0-9_]+)\(.*\n.*$!m', $response->data, $function_matches, PREG_SET_ORDER);
    foreach ($function_matches as $function_match) {
      $docblock = array(
        'object_name' => $function_match[1],
        'branch_name' => 'php',
        'object_type' => 'function',
        'file_name' => $location,
        'title' => $function_match[1],
        'summary' => api_documentation_summary($function_match[0]),
        'documentation' => $function_match[0],
        'code' => '');
      api_save_documentation($docblock);
    }
  }
}

/**
 * Read in the file at the given path and pass it off to the appropriate handler
 * for parsing.
 */
function api_parse_file($file_path, $branch_name, $file_name) {
  if (preg_match('!\.(php|module|inc|engine|theme)$!', $file_name)) {
    print('Parsing "'. $file_path .'"...<br />');
    api_parse_php_file($file_path, $branch_name, $file_name);
    return TRUE;
  }
  if (preg_match('!\.(txt)$!', $file_name)) {
    print('Parsing "'. $file_path .'"...<br />');
    api_parse_text_file($file_path, $branch_name, $file_name);
    return TRUE;
  }
  if (preg_match('!\.(htm|html)$!', $file_name)) {
    print('Parsing "'. $file_path .'"...<br />');
    api_parse_html_file($file_path, $branch_name, $file_name);
    return TRUE;
  }
  return FALSE;
}

/**
 * Read in the file at the given path and parse it as if it consisted entirely of
 * documentation.
 */
function api_parse_text_file($file_path, $branch_name, $file_name) {
  $source = file_get_contents($file_path);

  $docblock_matches = array();
  $docblocks = array();

  // Set up documentation block for file, in case it is not explicitly defined.
  $docblocks[0] = array(
      'object_name' => $file_name,
      'branch_name' => $branch_name,
      'object_type' => 'file',
      'file_name' => $file_name,
      'title' => strpos($file_name, '/') ? substr($file_name, strrpos($file_name, '/') + 1) : $file_name,
      'summary' => api_documentation_summary(api_format_documentation($source, $branch_name)),
      'documentation' => api_format_documentation($source, $branch_name),
      'code' => api_format_php($source));
  $docblocks[0]['version'] = '';
  $version_match = array();
  if (preg_match('!\$'.'Id: .*?,v (.*?) (.*?) (.*?) (.*?) Exp \$!', $source, $version_match)) {
    $docblocks[0]['version'] = $version_match[1] .' (checked in on '. $version_match[2] .' at '. $version_match[3] .' by '. $version_match[4] .')';
  }
  $docblocks[0]['modified'] = filemtime($file_path);

  $old_dids = array();
  $result = db_query("SELECT did FROM {api_documentation} WHERE branch_name = '%s' AND file_name = '%s'", $branch_name, $file_name);
  while ($object = db_fetch_object($result)) {
    $old_dids[] = $object->did;
  }

  $dids = array();
  foreach ($docblocks as $docblock) {
    $dids[] = api_save_documentation($docblock);
  }
  $old_dids = array_diff($old_dids, $dids);
  if (count($old_dids) > 0) {
    $old_dids = implode(',', $old_dids);
    db_query('DELETE FROM {api_documentation} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_file} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_function} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_reference} WHERE from_did IN (%s) OR to_did IN (%s)', $old_dids, $old_dids);
  }
}

/**
 * Read in the file at the given path and parse it as an HTML file.
 */
function api_parse_html_file($file_path, $branch_name, $file_name) {
  $source = file_get_contents($file_path);

  $docblocks = array();

  // Set up documentation block for file, in case it is not explicitly defined.
  $docblocks[0] = array(
      'object_name' => $file_name,
      'branch_name' => $branch_name,
      'object_type' => 'file',
      'file_name' => $file_name,
      'title' => strpos($file_name, '/') ? substr($file_name, strrpos($file_name, '/') + 1) : $file_name,
      'summary' => '',
      'documentation' => '',
      'code' => '<pre>'. htmlspecialchars($source) .'</pre>');
  $title_match = array();
  if (preg_match('!<title>(.*)</title>!is', $source, $title_match)) {
    $docblocks[0]['title'] = $title_match[1];
  }
  $documentation_match = array();
  if (preg_match('!<body>(.*?</h1>)?(.*)</body>!is', $source, $documentation_match)) {
    $docblocks[0]['documentation'] = $documentation_match[2];
    $docblocks[0]['summary'] = api_documentation_summary($documentation_match[2]);
  }

  $docblocks[0]['version'] = '';
  $version_match = array();
  if (preg_match('!\$'.'Id: .*?,v (.*?) (.*?) (.*?) (.*?) Exp \$!', $source, $version_match)) {
    $docblocks[0]['version'] = $version_match[1] .' (checked in on '. $version_match[2] .' at '. $version_match[3] .' by '. $version_match[4] .')';
  }
  $docblocks[0]['modified'] = filemtime($file_path);

  $old_dids = array();
  $result = db_query("SELECT did FROM {api_documentation} WHERE branch_name = '%s' AND file_name = '%s'", $branch_name, $file_name);
  while ($object = db_fetch_object($result)) {
    $old_dids[] = $object->did;
  }

  $dids = array();
  foreach ($docblocks as $docblock) {
    $dids[] = api_save_documentation($docblock);
  }
  $old_dids = array_diff($old_dids, $dids);
  if (count($old_dids) > 0) {
    $old_dids = implode(',', $old_dids);
    db_query('DELETE FROM {api_documentation} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_file} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_function} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_reference} WHERE from_did IN (%s) OR to_did IN (%s)', $old_dids, $old_dids);
  }
}

/**
 * Read in the file at the given path and parse its documentation.
 */
function api_parse_php_file($file_path, $branch_name, $file_name) {
  $source = file_get_contents($file_path);

  // Convert Mac/Win line breaks to Unix format.
  $source = str_replace("\r\n","\n", $source);
  $source = str_replace("\r","\n", $source);

  $docblock_matches = array();
  $docblocks = array();

  // Set up documentation block for file, in case it is not explicitly defined.
  $docblocks[0] = array(
      'object_name' => $file_name,
      'branch_name' => $branch_name,
      'object_type' => 'file',
      'file_name' => $file_name,
      'title' => strpos($file_name, '/') ? substr($file_name, strrpos($file_name, '/') + 1) : $file_name,
      'summary' => '',
      'documentation' => '',
      'code' => api_format_php($source));
  $docblocks[0]['version'] = '';
  $version_match = array();
  if (preg_match('!\$'.'Id: .*?,v (.*?) (.*?) (.*?) (.*?) Exp \$!', $source, $version_match)) {
    $docblocks[0]['version'] = $version_match[1] .' (checked in on '. $version_match[2] .' at '. $version_match[3] .' by '. $version_match[4] .')';
  }
  $docblocks[0]['modified'] = filemtime($file_path);

  $nested_groups = array();

  preg_match_all('!/\*\*(.*?)\*/!s', $source, $docblock_matches, PREG_SET_ORDER | PREG_OFFSET_CAPTURE);

  foreach ($docblock_matches as $docblock_match) {
    $docblock = array(
      'object_name' => '',
      'branch_name' => $branch_name,
      'object_type' => '',
      'file_name' => $file_name,
      'title' => '',
      'summary' => '',
      'documentation' => '',
      'code' => '');
    $docblock['content'] = str_replace(array("\n *", "\n "), array("\n", "\n"), $docblock_match[1][0]);
    $docblock['start'] = $docblock_match[0][1];
    $docblock['length'] = strlen($docblock_match[0][0]);

    // Determine what kind of documentation block this is.
    $code_start = $docblock['start'] + $docblock['length'] + 1;
    if (substr($source, $code_start, 8) == 'function') {
      $function_matches = array();

      $docblock['object_type'] = 'function';
      preg_match('!^function (([a-zA-Z0-9_]+)\(.*?) \{!', substr($source, $code_start), $function_matches);
      $docblock['object_name'] = $function_matches[2];
      $docblock['title'] = $function_matches[2];
      $docblock['signature'] = $function_matches[1];

      // We rely on the Drupal coding convention that functions are closed in column 1.
      $code_end = strpos($source, "\n}", $code_start) + 2;
      $docblock['code'] = substr($source, $code_start, $code_end - $code_start);
      $docblock['code'] = api_format_php("<?php\n". $docblock['code'] ."\n?>");

      $docblock['start_line'] = substr_count(substr($source, 0, $code_start), "\n");

      // Find parameter definitions.
      $param_match = array();
      $offset = 0;
      $docblock['parameters'] = '';
      while (preg_match('!@param(.*?)(?=\n@|\n\n|$)!s', substr($docblock['content'], $offset), $param_match, PREG_OFFSET_CAPTURE)) {
        $docblock['content'] = str_replace($param_match[0][0], '', $docblock['content']);
        $docblock['parameters'] .= "\n\n". $param_match[1][0];
        $offset = $param_match[0][1];
      }
      $docblock['parameters'] = api_format_documentation($docblock['parameters'], $branch_name);

      // Find return value definitions.
      $return_matches = array();
      $docblock['return'] = '';
      preg_match_all('!@return(.*?)(\n@|\n\n|$)!s', $docblock['content'], $return_matches, PREG_SET_ORDER);
      foreach($return_matches as $return_match) {
        $docblock['content'] = str_replace($return_match[0], '', $docblock['content']);
        $docblock['return'] .= "\n\n". $return_match[1];
      }
      $docblock['return'] = api_format_documentation($docblock['return'], $branch_name);

      // Find referenced functions.
      $function_call_matches = array();
      $docblock['function calls'] = array();
      preg_match_all('!<span class="php-function-or-constant">([a-zA-Z0-9_]+)</span>\(!', substr($docblock['code'], strlen('<div class="php"><code><span class="php-keyword">function</span> <span class="php-function-or-constant">')), $function_call_matches, PREG_SET_ORDER);
      foreach($function_call_matches as $function_call_match) {
        $docblock['function calls'][$function_call_match[1]] = $function_call_match[1];
      }

      // Determine group membership.
      $group_matches = array();
      preg_match_all('!@(ingroup|addtogroup) ([a-zA-Z0-9_]+)!', $docblock['content'], $group_matches);
      $docblock['groups'] = $group_matches[2];
      $docblock['content'] = preg_replace('!@ingroup.*?\n!', '', $docblock['content']);

      foreach ($nested_groups as $group_id) {
        if (!empty($group_id)) {
          $docblock['groups'][] = $group_id;
        }
      }
    }
    else if (substr($source, $code_start, 6) == 'define') {
      $constant_matches = array();

      $docblock['object_type'] = 'constant';
      preg_match('!^define\([\'"]([a-zA-Z0-9_]+)[\'"]!', substr($source, $code_start), $constant_matches);
      $docblock['object_name'] = $constant_matches[1];
      $docblock['title'] = $constant_matches[1];

      $code_end = strpos($source, ';', $code_start) + 1;
      $docblock['code'] = substr($source, $code_start, $code_end - $code_start);
      $docblock['code'] = api_format_php("<?php\n". $docblock['code'] ."\n?>");

      $docblock['start_line'] = substr_count(substr($source, 0, $code_start), "\n");

      // Determine group membership.
      $group_matches = array();
      preg_match_all('!@(ingroup|addtogroup) ([a-zA-Z0-9_]+)!', $docblock['content'], $group_matches);
      $docblock['groups'] = $group_matches[2];
      $docblock['content'] = preg_replace('!@ingroup.*?\n!', '', $docblock['content']);

      foreach ($nested_groups as $group_id) {
        if (!empty($group_id)) {
          $docblock['groups'][] = $group_id;
        }
      }
    }
    else if (strpos($docblock['content'], '@mainpage') !== FALSE) {
      $mainpage_matches = array();
      preg_match('!@mainpage (.*?)\n!', $docblock['content'], $mainpage_matches);
      $docblock['title'] = $mainpage_matches[1];
      $docblock['content'] = preg_replace('!@mainpage.*?\n!', '', $docblock['content']);
      $docblock['object_type'] = 'mainpage';
      $docblock['object_name'] = $branch_name;
    }
    else if (strpos($docblock['content'], '@file') !== FALSE) {
      $docblocks[0]['content'] = str_replace('@file', '', $docblock['content']);
      $docblocks[0]['documentation'] = api_format_documentation($docblocks[0]['content'], $branch_name);
      $docblocks[0]['summary'] = api_documentation_summary($docblocks[0]['documentation']);
    }
    else if (strpos($docblock['content'], '@defgroup') !== FALSE) {
      $group_matches = array();
      preg_match('!@defgroup ([a-zA-Z0-9_]+) +(.*?)\n!', $docblock['content'], $group_matches);
      $docblock['object_name'] = $group_matches[1];
      $docblock['title'] = $group_matches[2];
      $docblock['content'] = preg_replace('!@defgroup.*?\n!', '', $docblock['content']);
      $docblock['object_type'] = 'group';
    }

    // Handle nested function groups.
    if (strpos($docblock['content'], '@{') !== FALSE) {
      if ($docblock['object_type'] == 'group') {
        array_push($nested_groups, $docblock['object_name']);
      }
      else {
        $group_matches = array();
        if (preg_match('!@(ingroup|addtogroup) ([a-zA-Z0-9_]+)!', $docblock['content'], $group_matches)) {
          array_push($nested_groups, $group_matches[2]);
        }
        else {
          array_push($nested_groups, '');
        }
      }
    }
    if (strpos($docblock['content'], '@}') !== FALSE) {
      array_pop($nested_groups);
    }

    if ($docblock['object_type'] != '') {
      $docblock['documentation'] = api_format_documentation($docblock['content'], $branch_name);
      $docblock['summary'] = api_documentation_summary($docblock['documentation']);
      $docblocks[] = $docblock;
    }
  }

  // Find undocumented functions.
  $function_matches = array();
  preg_match_all('%(?<!\*/\n)^function (([a-zA-Z0-9_]+)\(.*?) \{%sm', $source, $function_matches, PREG_SET_ORDER | PREG_OFFSET_CAPTURE);
  foreach ($function_matches as $function_match) {
    $docblock = array(
      'object_name' => $function_match[2][0],
      'branch_name' => $branch_name,
      'object_type' => 'function',
      'file_name' => $file_name,
      'title' => $function_match[2][0],
      'summary' => '',
      'documentation' => '',
      'code' => '');
    $docblock['signature'] = $function_match[1][0];
    $docblock['parameters'] = '';
    $docblock['return'] = '';
    $docblock['groups'] = array();

    $code_start = $function_match[0][1];
    $code_end = strpos($source, "\n}", $code_start) + 2;
    $docblock['code'] = substr($source, $code_start, $code_end - $code_start);
    $docblock['code'] = api_format_php("<?php\n". $docblock['code'] ."\n?>");

    $docblock['start_line'] = substr_count(substr($source, 0, $code_start), "\n");

    $docblocks[] = $docblock;
  }

  // Find undocumented constants.
  $constant_matches = array();
  preg_match_all('%(?<!\*/\n)^define\([\'"]([a-zA-Z0-9_]+)[\'"]%sm', $source, $constant_matches, PREG_SET_ORDER | PREG_OFFSET_CAPTURE);
  foreach ($constant_matches as $constant_match) {
    $docblock = array(
      'object_name' => $constant_match[1][0],
      'branch_name' => $branch_name,
      'object_type' => 'constant',
      'file_name' => $file_name,
      'title' => $constant_match[1][0],
      'summary' => '',
      'documentation' => '',
      'code' => '');
    $docblock['groups'] = array();

    $code_start = $constant_match[0][1];
    $code_end = strpos($source, ';', $code_start) + 1;
    $docblock['code'] = substr($source, $code_start, $code_end - $code_start);
    $docblock['code'] = api_format_php("<?php\n". $docblock['code'] ."\n?>");

    $docblock['start_line'] = substr_count(substr($source, 0, $code_start), "\n");

    $docblocks[] = $docblock;
  }

  $old_dids = array();
  $result = db_query("SELECT did FROM {api_documentation} WHERE branch_name = '%s' AND file_name = '%s'", $branch_name, $file_name);
  while ($object = db_fetch_object($result)) {
    $old_dids[] = $object->did;
  }

  $dids = array();
  foreach ($docblocks as $docblock) {
    $dids[] = api_save_documentation($docblock);
  }
  $old_dids = array_diff($old_dids, $dids);
  if (count($old_dids) > 0) {
    $old_dids = implode(',', $old_dids);
    db_query('DELETE FROM {api_documentation} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_file} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_function} WHERE did IN (%s)', $old_dids);
    db_query('DELETE FROM {api_reference} WHERE from_did IN (%s) OR to_did IN (%s)', $old_dids, $old_dids);
  }
}

/**
 * Save a documentation block into the database.
 *
 * @param &$docblock
 *   An array containing information about the documentation block.
 * @return
 *   The documentation ID of the inserted/updated construct.
 */
function api_save_documentation(&$docblock) {
  $result = db_query("SELECT did FROM {api_documentation} WHERE object_name = '%s' AND branch_name = '%s' AND object_type = '%s'", $docblock['object_name'], $docblock['branch_name'], $docblock['object_type']);
  if (db_num_rows($result) > 0) {
    $did = db_result($result);
    db_query("UPDATE {api_documentation} SET title = '%s', file_name = '%s', summary = '%s', documentation = '%s', code = '%s' WHERE did = %d", $docblock['title'], $docblock['file_name'], $docblock['summary'], $docblock['documentation'], $docblock['code'], $did);
  }
  else {
    $did = db_next_id('{api_documentation}_did');
    db_query("INSERT INTO {api_documentation} (did, object_name, branch_name, object_type, title, file_name, summary, documentation, code) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", $did, $docblock['object_name'], $docblock['branch_name'], $docblock['object_type'], $docblock['title'], $docblock['file_name'], $docblock['summary'], $docblock['documentation'], $docblock['code']);
  }

  switch ($docblock['object_type']) {
    case 'function':
      db_query('DELETE FROM {api_function} WHERE did = %d', $did);
      db_query("INSERT INTO {api_function} (did, signature, start_line, parameters, `return`) VALUES (%d, '%s', %d, '%s', '%s')", $did, $docblock['signature'], $docblock['start_line'], $docblock['parameters'], $docblock['return']);

      if (is_array($docblock['function calls'])) {
        foreach ($docblock['function calls'] as $function_name) {
          api_reference('set', $docblock['branch_name'], 'function', $function_name, $did);
        }
      }
      break;

    case 'file':
      db_query('DELETE FROM {api_file} WHERE did = %d', $did);
      db_query("INSERT INTO {api_file} (did, modified, version) VALUES (%d, %d, '%s')", $did, $docblock['modified'], $docblock['version']);
      break;
  }

  if (is_array($docblock['groups'])) {
    foreach ($docblock['groups'] as $group_name) {
      api_reference('set', $docblock['branch_name'], 'group', $group_name, $did);
    }
  }

  return $did;
}

/**
 * Format a documentation block as HTML.
 */
function api_format_documentation($documentation, $branch_name) {
  // Don't do processing on empty text (so we don't end up with empty paragraphs).
  $documentation = trim($documentation);
  if (empty($documentation)) {
    return '';
  }

  $documentation = check_plain($documentation);

  // Process the @link tag.
  $documentation = preg_replace('!@link ([a-zA-Z0-9_/]+\.[a-zA-Z0-9_]+) (.*?) @endlink!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/file/$1')), $documentation);
  $documentation = preg_replace('!@link ([a-zA-Z0-9_]+)\(\) (.*?) @endlink!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/function/$1')), $documentation);
  $documentation = preg_replace('!@link ([a-zA-Z0-9_]+) (.*?) @endlink!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/group/$1')), $documentation);

  // Process the @see tag.
  $documentation = preg_replace('!@see ([a-zA-Z0-9_]+) (.*?) \n!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/group/$1')), $documentation);
  $documentation = preg_replace('!@see ([a-zA-Z0-9_]+)\(\) (.*?) \n!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/function/$1')), $documentation);
  $documentation = preg_replace('!@see ([a-zA-Z0-9_]+) (.*?) \n!', str_replace('%24', '$', l('$2', 'api/'. $branch_name .'/group/$1')), $documentation);

  // Remove any leftover Doxygen formatting statements.
  $documentation = preg_replace('!@[a-zA-Z0-9_}{]+!', '', $documentation);

  $documentation = api_format_documentation_lists($documentation);

  // Convert newlines into paragraphs.
  $documentation = preg_replace('|\n*$|', '', $documentation) ."\n\n"; // just to make things a little easier, pad the end
  $documentation = preg_replace('!\n?(.+?)(?:\n\s*\n|\z)!s', "<p>$1</p>\n", $documentation); // make paragraphs, including one at the end
  $documentation = preg_replace('!<p>\s*?</p>!', '', $documentation); // under certain strange conditions it could create a P of entirely whitespace

  return $documentation;
}

/**
 * Parse a block of text for lists that use hyphens or asterisks as bullets, and
 * format them as proper HTML lists.
 */
function api_format_documentation_lists($documentation) {
  $lines = explode("\n", $documentation);
  $output = '';
  $bullet_indents = array(-1);

  foreach ($lines as $line) {
    $matches = array();
    preg_match('!^( *)([*-] )?(.*)$!', $line, $matches);
    $indent = strlen($matches[1]);
    $bullet_exists = $matches[2];

    if ($indent < $bullet_indents[0]) {
      // First close off any lists that have completed.
      while ($indent < $bullet_indents[0]) {
        array_shift($bullet_indents);
        $output .= '</li></ul>';
      }
    }

    if ($indent == $bullet_indents[0]) {
      if ($bullet_exists) {
        // A new bullet at the same indent means a new list item.
        $output .= '</li><li>';
      }
      else {
        // If the indent is the same, but there is no bullet, that also
        // signifies the end of the list.
        array_shift($bullet_indents);
        $output .= '</li></ul>';
      }
    }

    if ($indent > $bullet_indents[0] && $bullet_exists) {
      // A new list at a lower level.
      array_unshift($bullet_indents, $indent);
      $output .= '<ul><li>';
    }

    $output .= $matches[3] ."\n";
  }

  // Clean up any unclosed lists.
  array_pop($bullet_indents);
  foreach ($bullet_indents as $indent) {
    $output .= '</li></ul>';
  }

  return $output;
}

/**
 * Retrieve a summary from a documentation block.
 */
function api_documentation_summary($documentation) {
  $pos = strpos($documentation, "</p>");
  if ($pos !== FALSE) {
    $documentation = substr($documentation, 0, $pos);
  }
  $documentation = trim(strip_tags($documentation));

  if (strlen($documentation) > 255) {
    return substr($documentation, 0, strrpos(substr($documentation, 0, 252), ' ')) .'...';
  }
  else {
    return $documentation;
  }
}

/**
 * Colorize and format a PHP script.
 */
function api_format_php($code) {
  $output = '';

  if (!defined('T_ML_COMMENT')) {
    define('T_ML_COMMENT', T_COMMENT);
  }
  if (!defined('T_DOC_COMMENT')) {
    define('T_DOC_COMMENT', T_COMMENT);
  }

  $tokens = token_get_all($code);

  $in_string = FALSE;

  foreach($tokens as $token) {
    if ($in_string) {
      if ($token == '"') {
        $output .= '"</span>';
        $in_string = FALSE;
      }
      else {
        $output .= is_array($token) ? htmlspecialchars($token[1]) : htmlspecialchars($token);
      }
      continue;
    }
    else if ($token == '"') {
      $output .= '<span class="php-string">"';
      $in_string = TRUE;
      continue;
    }

    if (is_array($token)) {
      $type = $token[0];
      $value = htmlspecialchars($token[1]);

      switch ($type) {
        // Discarded strings:
        case T_OPEN_TAG:
        case T_CLOSE_TAG:
          break;

        case T_COMMENT:
        case T_ML_COMMENT:
        case T_DOC_COMMENT:
          $output .= '<span class="php-comment">'. $value .'</span>';
          break;

        case T_VARIABLE:
          $output .= '<span class="php-variable">'. $value .'</span>';
          break;

        case T_CONSTANT_ENCAPSED_STRING:
        case T_INLINE_HTML:
          $output .= '<span class="php-string">'. $value .'</span>';
          break;

        case T_STRING:
          $output .= '<span class="php-function-or-constant">'. $value .'</span>';
          break;

        case T_LNUMBER:
        case T_DNUMBER:
          $output .= '<span class="php-constant">'. $value .'</span>';
          break;

        case T_ARRAY_CAST: case T_ARRAY: case T_AS: case T_BOOL_CAST:
        case T_BREAK: case T_CASE: case T_CLASS: case T_CONST:
        case T_CONTINUE: case T_DECLARE: case T_DEFAULT: case T_DO:
        case T_DOUBLE_CAST: case T_ECHO: case T_ELSE: case T_ELSEIF:
        case T_EMPTY: case T_ENDDECLARE: case T_ENDFOR: case T_ENDFOREACH:
        case T_ENDIF: case T_ENDSWITCH: case T_ENDWHILE: case T_EVAL:
        case T_EXIT: case T_EXTENDS: case T_FOR: case T_FOREACH:
        case T_FUNCTION: case T_GLOBAL: case T_IF: case T_INCLUDE_ONCE:
        case T_INCLUDE: case T_INT_CAST: case T_ISSET: case T_LIST:
        case T_NEW: case T_OBJECT_CAST: case T_PRINT:
        case T_REQUIRE_ONCE: case T_REQUIRE: case T_RETURN: case T_STATIC:
        case T_STRING_CAST: case T_SWITCH: case T_UNSET_CAST: case T_UNSET:
        case T_USE: case T_VAR: case T_WHILE:
          $output .= '<span class="php-keyword">'. $value .'</span>';
          break;

        default:
          $output .= $value;
      }
    }
    else {
      $output .= $token;
    }
  }

  // Manage whitespace:
  $output = preg_replace('!([\n ]+)!se', 'api_format_php_whitespace(\'$1\')', $output);
  return '<div class="php"><code>'. $output .'</code></div>';
}

/**
 * Given a run of whitespace, convert it into nonbreaking spaces and forced line
 * breaks as necessary to preserve it in the resulting HTML.
 */
function api_format_php_whitespace($whitespace) {
  $output = '';
  $breakingspace = 1;
  $runlength = 0;
  for ($i = 0; $i < strlen($whitespace); $i++) {
    if ($whitespace{$i} == "\n") {
      for ($j = 0; $j < $runlength - $breakingspace; $j++) {
        $output .= '&nbsp;';
      }
      if ($breakingspace) {
        $output .= ' ';
      }
      $output .= "<br />\n";
      $breakingspace = 0;
    }
    else {
      $runlength++;
    }
  }
  for ($j = 0; $j < $runlength - $breakingspace; $j++) {
    $output .= '&nbsp;';
  }
  if ($breakingspace) {
    $output .= ' ';
  }

  return $output;
}

/**
 * Since we may parse a file containing a reference before we have parsed the
 * file containing the referenced object, we manage all references internally
 * in an array and store the array in a Drupal persistent variable. Then we can
 * save the references to the database table when the referenced object has
 * been parsed.
 */
function api_reference($op, $branch_name = '', $to_type = '', $to_name = '', $from_did = 0) {
  static $reference = NULL;

  if ($reference == NULL) {
    $reference = variable_get('api_reference', array());
  }

  switch ($op) {
    case 'save':
      foreach (array_keys($reference) as $key) {
        list($branch_name, $to_type, $to_name) = unserialize($key);
        if ($to_did = db_result(db_query("SELECT did FROM {api_documentation} WHERE object_name = '%s' AND branch_name = '%s' AND object_type = '%s'", $to_name, $branch_name, $to_type))) {
          foreach ($reference[$key] as $from_did) {
            db_query('REPLACE INTO {api_reference} (from_did, to_did) VALUES (%d, %d)', $from_did, $to_did);
          }
          unset($reference[$key]);
        }
        else if (db_result(db_query("SELECT COUNT(*) FROM {api_documentation} WHERE object_name = '%s' AND branch_name = 'php' AND object_type = '%s'", $to_name, $to_type))) {
          unset($reference[$key]);
        }
      }

      variable_set('api_reference', $reference);
      break;
    case 'set':
      $key = serialize(array($branch_name, $to_type, $to_name));
      $reference[$key][] = $from_did;
      break;
  }
}

?>
