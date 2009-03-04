<?php
define ('T_NONE', -1);

function parse_module($file) {
  $tokens = token_get_all(file_get_contents($file));
  $line_number = 0;
  $function = array();
  while (($token = next($tokens)) !== FALSE) {
    if (!is_array($token)) {
      $token = array(T_NONE, $token, $line_number);
    }
    list($type, $value, $line_number) = $token;
    switch ($type) {
      case T_DOC_COMMENT:
        // this is the doxygen / group / file definition. create a docblock.
        // Save the docblock for functions.
        $doxygen = str_replace(' *   ', '', $value);
        break;
      case T_FUNCTION:
        while (($token = next($tokens)) !== FALSE && $block_depth !== 0) {
          if (!is_array($token)) {
            $token = array(T_NONE, $token, $line_number);
          }
          list($type, $value, $line_number) = $token;
          if (empty($function['name'])) {
            // If function name hasn't been set then look for first string.
            if ($type == T_STRING) {
              $function['name'] = $value;
              $current_parameter = '';
            }
            // Note that if $doxygen is empty at this point then the function
            // is undocumented.
          }
          else if (!isset($block_depth)) {
            // If function body hasn't started than look for parameters
            switch ($type) {
              case T_VARIABLE:
                $current_parameter = ($reference ? '&' : '') . $value;
                $function['parameters'][$current_parameter] = NULL;
                preg_match('#@param \$'. substr($value, 1) ."\n((?:.(?! * @| */))+)#s", $doxygen, $matches);
                // $value is the parameter and $matches[1] or trim($matches[1]) is the documentation for it.
                $reference = FALSE;
                break;
              case T_STRING:
              case T_CONSTANT_ENCAPSED_STRING:
              case T_LNUMBER:
              case T_DNUMBER:
                $function['parameters'][$current_parameter] = $value;
                break;
              case T_NONE:
                if ($value == '{') {
                  $block_depth = 1;
                }
                else if ($value == '&') {
                  $reference = TRUE;
                }
                break;
            }
          }
          else {
            // Look for internal function calls.
            switch ($type) {
              case T_STRING:
                $function['calls'][$value] = $value;
                $open_call = $value;
                break;
              case T_NONE:
                if ($open_call && $value == ')') {
                  $open_call = FALSE;
                }
                else if ($value == '{') {
                  $block_depth++;
                }
                else if ($value == '}') {
                  $block_depth--;
                }
                break;
            }
          }
        }
        $function = array();
        $doxygen = '';
        unset($block_depth);
    }
  }
}
parse_module($argv[1]);