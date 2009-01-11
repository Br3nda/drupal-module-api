<p><?= api_file_link($function) ?>, <?= t('line') ?> <?= $function->start_line ?></p>

<dl id="api-function-signature">
<?php foreach ($signatures as $branch => $signature) { ?>
  <?php if ($signature['active']) { ?>
    <dt style="width: <?= $branch_length ?>em"><strong><?= $branch ?></strong></dt>
    <dd style="margin-left: <?= $branch_length + 0.5 ?>em"><strong><code><?= $signature['signature'] ?></code></strong></dd>
  <?php } else { ?>
    <dt style="width: <?= $branch_length ?>em"><?= l($branch, $signature['url']) ?></dt>
    <dd style="margin-left: <?= $branch_length + 0.5 ?>em"><code><?= l($signature['signature'], $signature['url'], array('html' => TRUE)) ?></code></dd>
  <?php } ?>
<?php } ?>
</dl>

<?= $documentation ?>

<?php if (!empty($parameters)) { ?>
<h3><?= t('Parameters') ?></h3>
<?= $parameters ?>
<?php } ?>

<?php if (!empty($return)) { ?>
<h3><?= t('Return value') ?></h3>
<?= $return ?>
<?php } ?>

<?php if (!empty($related_topics)) { ?>
<h3><?= t('Related topics') ?></h3>
<?= $related_topics ?>
<?php } ?>

<?php if (!empty($call)) { ?>
<h3><?= t('Functions that call @name()', array('@name' => $function->title)) ?></h3>
<?= $call ?>
<?php } ?>

<?php if (!empty($called)) { ?>
<h3><?= t('Functions called by @name()', array('@name' => $function->title)) ?></h3>
<?= $called ?>
<?php } ?>

<?php if (!empty($code)) { ?>
<h3><?= t('Code') ?></h3>
<?= $code ?>
<?php } ?>
