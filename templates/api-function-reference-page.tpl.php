<?php if (!empty($related_topics)) { ?>
<h3><?= t('Related topics') ?></h3>
<?= $related_topics ?>
<?php } ?>

<?php if (!empty($call)) { ?>
<h3><?= t('Functions that call @name()', array('@name' => $function)) ?></h3>
<?= $call ?>
<?php } ?>

<?php if (!empty($called)) { ?>
<h3><?= t('Functions called by @name()', array('@name' => $function)) ?></h3>
<?= $called ?>
<?php } ?>
